/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "render_input_postprocess.h"
#include "texture_interface.h"
#include "graphics_camera.h"
#include "graphicsstate.h"
#include "glutil.h"
#include "matrix4.h"
#include "shader.h"
#include "uniforms.h"
#include "drawable.h"
#include "vertexbuffer.h"

RenderInputPostprocess::RenderInputPostprocess(VertexBuffer & buffer, Drawable & quad) :
	vertex_buffer(buffer),
	screen_quad(quad),
	shadow_matrix(NULL),
	shadow_count(0),
	shader(NULL),
	contrast(1)
{
	//ctor
}

RenderInputPostprocess::~RenderInputPostprocess()
{
	//dtor
}

void RenderInputPostprocess::SetShader(Shader * newshader)
{
	shader = newshader;
}

void RenderInputPostprocess::ClearOutput(GraphicsState & glstate, bool color, bool depth)
{
	glstate.ClearDrawBuffer(color, depth);
}

void RenderInputPostprocess::SetColorMask(GraphicsState & glstate, bool write_color, bool write_alpha)
{
	glstate.ColorMask(write_color, write_alpha);
}

void RenderInputPostprocess::SetDepthMode(GraphicsState & glstate, int mode, bool write_depth)
{
	glstate.DepthTest(mode, write_depth);
}

void RenderInputPostprocess::SetBlendMode(GraphicsState & glstate, BlendMode::Enum mode)
{
	switch (mode)
	{
		case BlendMode::DISABLED:
		{
			glstate.Blend(false);
		}
		break;

		case BlendMode::ADD:
		{
			glstate.Blend(true);
			glstate.BlendFunc(GL_ONE, GL_ONE);
		}
		break;

		case BlendMode::ALPHABLEND:
		{
			glstate.Blend(true);
			glstate.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		case BlendMode::PREMULTIPLIED_ALPHA:
		{
			glstate.Blend(true);
			glstate.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		default:
		assert(0);
		break;
	}
}

void RenderInputPostprocess::SetShadowMatrix(const Mat4 shadow_mat[], const unsigned int count)
{
	shadow_matrix = shadow_mat;
	shadow_count = count;
}

void RenderInputPostprocess::SetTextures(
	GraphicsState & glstate,
	const std::vector <TextureInterface*> & textures,
	std::ostream & error_output)
{
	unsigned int num_nonnull = 0;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		if (textures[i])
		{
			glstate.BindTexture(i, textures[i]->GetTarget(), textures[i]->GetId());
			num_nonnull++;
		}
	}
	if (textures.size() && !num_nonnull)
	{
		error_output << "Out of the " << textures.size() << " input textures provided as inputs to this postprocess stage, zero are available. This stage will have no effect." << std::endl;
		return;
	}
	CheckForOpenGLErrors("postprocess texture set", error_output);
}

void RenderInputPostprocess::SetCamera(const GraphicsCamera & cam)
{
	cam_rotation = cam.rot;
	cam_position = cam.pos;

	// get frustum far plane corners
	const Mat4 proj_inv = GetProjMatrixInv(cam);
	const float lod_far = cam.view_distance;
	frustum_corners[0].Set(-lod_far, -lod_far, -lod_far); // BL
	frustum_corners[1].Set( lod_far, -lod_far, -lod_far); // BR
	frustum_corners[2].Set( lod_far,  lod_far, -lod_far); // TR
	frustum_corners[3].Set(-lod_far,  lod_far, -lod_far); // TL
	for (int i = 0; i < 4; i++)
	{
		proj_inv.TransformVectorOut(frustum_corners[i][0], frustum_corners[i][1], frustum_corners[i][2]);
		frustum_corners[i][2] = -lod_far;
	}

	// frustum corners in world space for dynamic sky shader
	Mat4 view_rot_inv;
	(-cam.rot).GetMatrix4(view_rot_inv);
	for (int i = 0; i < 4; i++)
	{
		frustum_corners_ws[i] = frustum_corners[i];
		view_rot_inv.TransformVectorOut(frustum_corners_ws[i][0], frustum_corners_ws[i][1], frustum_corners_ws[i][2]);
	}
}

void RenderInputPostprocess::SetSunDirection(const Vec3 & newsun)
{
	lightposition = newsun;
}

void RenderInputPostprocess::SetContrast(float value)
{
	contrast = value;
}

void RenderInputPostprocess::Render(GraphicsState & glstate, std::ostream & error_output)
{
	CheckForOpenGLErrors("postprocess begin", error_output);

	assert(shader && "RenderInputPostprocess::Render No shader set.");

	shader->Enable();

	CheckForOpenGLErrors("postprocess shader enable", error_output);

	glstate.ActiveTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	CheckForOpenGLErrors("postprocess flag set", error_output);

	Mat4 proj_matrix, view_matrix, view_proj_matrix;
	proj_matrix.SetOrthographic(0, 1, 0, 1, -1, 1);
	view_matrix.LoadIdentity();
	view_proj_matrix = view_matrix.Multiply(proj_matrix);

	Quat cam_look;
	cam_look.Rotate(M_PI_2, 1, 0, 0);
	cam_look.Rotate(-M_PI_2, 0, 0, 1);
	Quat cube_rotation;
	cube_rotation = (-cam_look) * (-cam_rotation); // experimentally derived
	float cube_matrix[9];
	cube_rotation.GetMatrix3(cube_matrix);

	Vec3 lightvec = lightposition;
	cam_rotation.RotateVector(lightvec);

	const float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	if (shadow_matrix)
	{
		shader->SetUniformMat4f(Uniforms::ShadowMatrix, shadow_matrix[0].GetArray(), shadow_count);
	}
	shader->SetUniformMat4f(Uniforms::ModelViewProjMatrix, view_proj_matrix.GetArray());
	shader->SetUniformMat4f(Uniforms::ModelViewMatrix, view_matrix.GetArray());
	shader->SetUniformMat4f(Uniforms::ProjectionMatrix, proj_matrix.GetArray());
	shader->SetUniformMat3f(Uniforms::ReflectionMatrix, cube_matrix);
	shader->SetUniform3f(Uniforms::LightDirection, lightvec);
	shader->SetUniform4f(Uniforms::ColorTint, color);
	shader->SetUniform1f(Uniforms::Contrast, contrast);
	shader->SetUniform1f(Uniforms::ZNear, 0.1);
	shader->SetUniform3f(Uniforms::FrustumCornerBL, frustum_corners[0]);
	shader->SetUniform3f(Uniforms::FrustumCornerBRDelta, frustum_corners[1] - frustum_corners[0]);
	shader->SetUniform3f(Uniforms::FrustumCornerTLDelta, frustum_corners[3] - frustum_corners[0]);

	vertex_buffer.Draw(glstate.VertexObject(), screen_quad.GetVertexBufferSegment());

	CheckForOpenGLErrors("postprocess draw", error_output);
}
