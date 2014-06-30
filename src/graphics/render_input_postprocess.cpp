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
#include "vertexattrib.h"

RenderInputPostprocess::RenderInputPostprocess() :
	shader(NULL),
	contrast(1),
	maxu(1),
	maxv(1)
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

void RenderInputPostprocess::SetTextures(
	GraphicsState & glstate,
	const std::vector <TextureInterface*> & textures,
	std::ostream & error_output)
{
	maxu = 1;
	maxv = 1;
	int num_nonnull = 0;
	for (unsigned i = 0; i < textures.size(); i++)
	{
		const TextureInterface * t = textures[i];
		if (t)
		{
			glstate.BindTexture(i, t->GetTarget(), t->GetId());
			if (t->GetTarget() == GL_TEXTURE_RECTANGLE)
			{
				maxu = t->GetW();
				maxv = t->GetH();
			}
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
	assert(shader);

	CheckForOpenGLErrors("postprocess begin", error_output);

	shader->Enable();

	CheckForOpenGLErrors("postprocess shader enable", error_output);

	Mat4 projMatrix, viewMatrix;
	projMatrix.SetOrthographic(0, 1, 0, 1, -1, 1);
	viewMatrix.LoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projMatrix.GetArray());

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(viewMatrix.GetArray());

	glstate.ActiveTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	CheckForOpenGLErrors("postprocess flag set", error_output);

	// send shader parameters
	{
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

		shader->SetUniform3f(Uniforms::LightDirection, lightvec);
		shader->SetUniform4f(Uniforms::ColorTint, color);
		shader->SetUniform1f(Uniforms::Contrast, contrast);
		shader->SetUniformMat3f(Uniforms::ReflectionMatrix, cube_matrix);
		shader->SetUniform1f(Uniforms::ZNear, 0.1);
		shader->SetUniform3f(Uniforms::FrustumCornerBL, frustum_corners[0]);
		shader->SetUniform3f(Uniforms::FrustumCornerBRDelta, frustum_corners[1] - frustum_corners[0]);
		shader->SetUniform3f(Uniforms::FrustumCornerTLDelta, frustum_corners[3] - frustum_corners[0]);
	}

	// draw a quad
	const unsigned faces[2 * 3] = {
		0, 1, 2,
		2, 3, 0,
	};
	const float pos[4 * 3] = {
		0.0f,  0.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};
	// send the uv corners as tex coords
	const float tco[4 * 2] = {
		0.0f, 0.0f,
		maxu, 0.0f,
		maxu, maxv,
		0.0f, maxv,
	};
	// send the frustum corners as normal
	const float fcl[4 * 3] = {
		frustum_corners[0][0], frustum_corners[0][1], frustum_corners[0][2],
		frustum_corners[1][0], frustum_corners[1][1], frustum_corners[1][2],
		frustum_corners[2][0], frustum_corners[2][1], frustum_corners[2][2],
		frustum_corners[3][0], frustum_corners[3][1], frustum_corners[3][2],
	};
	// fructum corners in world space as tangent
	const float fcw[4 * 3] = {
		frustum_corners_ws[0][0], frustum_corners_ws[0][1], frustum_corners_ws[0][2],
		frustum_corners_ws[1][0], frustum_corners_ws[1][1], frustum_corners_ws[1][2],
		frustum_corners_ws[2][0], frustum_corners_ws[2][1], frustum_corners_ws[2][2],
		frustum_corners_ws[3][0], frustum_corners_ws[3][1], frustum_corners_ws[3][2],
	};

	if (glstate.VertexObject())
		glstate.ResetVertexObject();

	using namespace VertexAttrib;

	glEnableVertexAttribArray(VertexPosition);
	glEnableVertexAttribArray(VertexTexCoord);
	glEnableVertexAttribArray(VertexNormal);
	glEnableVertexAttribArray(VertexTangent);

	glVertexAttribPointer(VertexPosition, 3, GL_FLOAT, GL_FALSE, 0, pos);
	glVertexAttribPointer(VertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0, tco);
	glVertexAttribPointer(VertexNormal, 3, GL_FLOAT, GL_FALSE, 0, fcl);
	glVertexAttribPointer(VertexTangent, 3, GL_FLOAT, GL_FALSE, 0, fcw);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, faces);

	glDisableVertexAttribArray(VertexPosition);
	glDisableVertexAttribArray(VertexTexCoord);
	glDisableVertexAttribArray(VertexNormal);
	glDisableVertexAttribArray(VertexTangent);

	CheckForOpenGLErrors("postprocess draw", error_output);
}
