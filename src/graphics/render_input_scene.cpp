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

#include "render_input_scene.h"
#include "texture_interface.h"
#include "graphics_camera.h"
#include "graphicsstate.h"
#include "drawable.h"
#include "shader.h"
#include "uniforms.h"
#include "glutil.h"

RenderInputScene::RenderInputScene(VertexBuffer & buffer):
	vertex_buffer(buffer),
	shadow_matrix(NULL),
	shadow_count(0),
	shader(NULL),
	lod_far(1000),
	fsaa(0),
	contrast(1.0)
{
	lightposition = Vec3(1, 0, 0);
	Quat ldir;
	ldir.Rotate(3.141593 * 0.5, 1, 0, 0);
	ldir.RotateVector(lightposition);
}

RenderInputScene::~RenderInputScene()
{
	//dtor
}

void RenderInputScene::SetShader(Shader & newshader)
{
	shader = &newshader;
}

void RenderInputScene::SetFSAA(unsigned value)
{
	fsaa = value;
}

void RenderInputScene::ClearOutput(GraphicsState & glstate, bool color, bool depth)
{
	glstate.ClearDrawBuffer(color, depth);
}

void RenderInputScene::SetColorMask(GraphicsState & glstate, bool write_color, bool write_alpha)
{
	glstate.ColorMask(write_color, write_alpha);
}

void RenderInputScene::SetDepthMode(GraphicsState & glstate, int mode, bool write_depth)
{
	glstate.DepthTest(mode, write_depth);
}

void RenderInputScene::SetBlendMode(GraphicsState & glstate, BlendMode::Enum mode)
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
			glstate.BlendFunc(GL_SRC_ALPHA, GL_ONE);
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

void RenderInputScene::SetShadowMatrix(const Mat4 shadow_mat[], const unsigned int count)
{
	shadow_matrix = shadow_mat;
	shadow_count = count;
}

void RenderInputScene::SetTextures(
	GraphicsState & glstate,
	const std::vector <TextureInterface*> & textures,
	std::ostream & error_output)
{
	for (unsigned i = 0; i < textures.size(); i++)
	{
		if (textures[i])
		{
			glstate.BindTexture(i, textures[i]->GetTarget(), textures[i]->GetId());

			if (CheckForOpenGLErrors("RenderDrawlists extra texture bind", error_output))
			{
				error_output << "this error occurred while binding texture " << i << " id=" << textures[i]->GetId() << std::endl;
			}
		}
	}
}

void RenderInputScene::SetCamera(const GraphicsCamera & cam)
{
	cam_position = cam.pos;
	cam_rotation = cam.rot;
	lod_far = cam.view_distance;

	projMatrix = GetProjMatrix(cam);
	viewMatrix = GetViewMatrix(cam);
	frustum.Extract(projMatrix.GetArray(), viewMatrix.GetArray());
}

void RenderInputScene::SetSunDirection(const Vec3 & newsun)
{
	lightposition = newsun;
}

void RenderInputScene::SetContrast(float value)
{
	contrast = value;
}

void RenderInputScene::SetDrawList(const std::vector <Drawable*> & drawlist)
{
	drawlist_ptr = &drawlist;
}

void RenderInputScene::Render(GraphicsState & glstate, std::ostream & /*error_output*/)
{
	assert(shader && "RenderInputScene::Render No shader set.");

	// invalidate color and transform
	drawable_color = Vec4(-1.0f);
	drawable_transform.Scale(0.0);

	Quat cam_look;
	cam_look.Rotate(M_PI_2, 1, 0, 0);
	cam_look.Rotate(-M_PI_2, 0, 0, 1);
	Quat cube_rotation;
	cube_rotation = (-cam_look) * (-cam_rotation); // experimentally derived
	float cube_matrix[9];
	cube_rotation.GetMatrix3(cube_matrix);

	Vec3 lightvec = lightposition;
	(cam_rotation).RotateVector(lightvec);

	shader->Enable();
	if (shadow_matrix)
	{
		shader->SetUniformMat4f(Uniforms::ShadowMatrix, shadow_matrix[0].GetArray(), shadow_count);
	}
	shader->SetUniformMat4f(Uniforms::ProjectionMatrix, projMatrix.GetArray());
	shader->SetUniformMat3f(Uniforms::ReflectionMatrix, cube_matrix);
	shader->SetUniform3f(Uniforms::LightDirection, lightvec[0], lightvec[1], lightvec[2]);
	shader->SetUniform1f(Uniforms::Contrast, contrast);

	Draw(glstate, *drawlist_ptr);
}

void RenderInputScene::Draw(GraphicsState & glstate, const std::vector <Drawable*> & drawlist)
{
	for (std::vector <Drawable*>::const_iterator ptr = drawlist.begin(); ptr != drawlist.end(); ++ptr)
	{
		const Drawable & d = **ptr;
		SetFlags(d, glstate);
		SetTextures(d, glstate);
		SetTransform(d);
		vertex_buffer.Draw(glstate.VertexObject(), d.GetVertexBufferSegment());
	}
}

void RenderInputScene::SetFlags(const Drawable & d, GraphicsState & glstate)
{
	glstate.DepthOffset(d.GetDecal());
	glstate.CullFace(d.GetCull());
	if (drawable_color != d.GetColor())
	{
		drawable_color = d.GetColor();
		shader->SetUniform4f(Uniforms::ColorTint, &drawable_color[0]);
	}
}

void RenderInputScene::SetTextures(const Drawable & d, GraphicsState & glstate)
{
	glstate.BindTexture(0, GL_TEXTURE_2D, d.GetTexture0());
	glstate.BindTexture(1, GL_TEXTURE_2D, d.GetTexture1());
	glstate.BindTexture(2, GL_TEXTURE_2D, d.GetTexture2());
}

void RenderInputScene::SetTransform(const Drawable & d)
{
	if (!drawable_transform.Equals(d.GetTransform()))
	{
		drawable_transform = d.GetTransform();
		const Mat4 mv = drawable_transform.Multiply(viewMatrix);
		const Mat4 mvp = mv.Multiply(projMatrix);
		shader->SetUniformMat4f(Uniforms::ModelViewProjMatrix, mvp.GetArray());
		shader->SetUniformMat4f(Uniforms::ModelViewMatrix, mv.GetArray());
	}
}
