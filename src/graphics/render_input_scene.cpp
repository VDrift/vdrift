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
#include "vertexattrib.h"
#include "vertexarray.h"
#include "glutil.h"

RenderInputScene::RenderInputScene(VertexBuffer & buffer):
	vertex_buffer(buffer),
	last_transform_valid(false),
	lod_far(1000),
	shader(NULL),
	fsaa(0),
	contrast(1.0)
{
	Vec3 front(1,0,0);
	lightposition = front;
	Quat ldir;
	ldir.Rotate(3.141593*0.5,1,0,0);
	ldir.RotateVector(lightposition);
}

RenderInputScene::~RenderInputScene()
{
	//dtor
}

void RenderInputScene::SetShader(Shader * newshader)
{
	shader = newshader;
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

void RenderInputScene::SetDrawLists(
	const std::vector <Drawable*> & dl_dynamic,
	const std::vector <Drawable*> & dl_static)
{
	dynamic_drawlist_ptr = &dl_dynamic;
	static_drawlist_ptr = &dl_static;
}

void RenderInputScene::Render(GraphicsState & glstate, std::ostream & error_output)
{
	assert(shader && "RenderInputScene::Render No shader set.");

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projMatrix.GetArray());

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(viewMatrix.GetArray());

	drawable_color = Vec4(-1.0f); // invalidate color

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
	shader->SetUniform3f(Uniforms::LightDirection, lightvec[0], lightvec[1], lightvec[2]);
	shader->SetUniform4f(Uniforms::ColorTint, &drawable_color[0]);
	shader->SetUniform1f(Uniforms::Contrast, contrast);
	shader->SetUniformMat3f(Uniforms::ReflectionMatrix, cube_matrix);

	last_transform_valid = false;

	Draw(glstate, *dynamic_drawlist_ptr, false);
	Draw(glstate, *static_drawlist_ptr, true);
}

void RenderInputScene::Draw(GraphicsState & glstate, const std::vector <Drawable*> & drawlist, bool preculled)
{
	for (std::vector <Drawable*>::const_iterator ptr = drawlist.begin(); ptr != drawlist.end(); ++ptr)
	{
		const Drawable & d = **ptr;
		if (preculled || !FrustumCull(d))
		{
			SetFlags(d, glstate);

			SetTextures(d, glstate);

			SetTransform(d, glstate);

			if (!d.GetVertArray())
			{
				vertex_buffer.Draw(glstate.VertexObject(), d.GetVertexBufferSegment());
			}
			else
			{
				if (glstate.VertexObject())
					glstate.ResetVertexObject();

				DrawVertexArray(*d.GetVertArray(), d.GetLineSize());
			}
		}
	}
}

void RenderInputScene::DrawVertexArray(const VertexArray & va, float linesize) const
{
	using namespace VertexAttrib;

	const float * verts;
	int vcount;
	va.GetVertices(verts, vcount);
	if (verts)
	{
		glVertexAttribPointer(VertexPosition, 3, GL_FLOAT, GL_FALSE, 0, verts);
		glEnableVertexAttribArray(VertexPosition);

		const unsigned char * cols;
		int ccount;
		va.GetColors(cols, ccount);
		if (cols)
		{
			glVertexAttribPointer(VertexColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cols);
			glEnableVertexAttribArray(VertexColor);
		}

		const int * faces;
		int fcount;
		va.GetFaces(faces, fcount);
		if (faces)
		{
			const float * norms;
			int ncount;
			va.GetNormals(norms, ncount);
			if (norms)
			{
				glVertexAttribPointer(VertexNormal, 3, GL_FLOAT, GL_FALSE, 0, norms);
				glEnableVertexAttribArray(VertexNormal);
			}

			const float * tcos = 0;
			int tcount;
			va.GetTexCoords(tcos, tcount);
			if (tcos)
			{
				glVertexAttribPointer(VertexTexCoord, 2, GL_FLOAT, GL_FALSE, 0, tcos);
				glEnableVertexAttribArray(VertexTexCoord);
			}

			glDrawElements(GL_TRIANGLES, fcount, GL_UNSIGNED_INT, faces);

			if (tcos)
				glDisableVertexAttribArray(VertexTexCoord);

			if (norms)
				glDisableVertexAttribArray(VertexNormal);
		}
		else if (linesize > 0)
		{
			glLineWidth(linesize);
			glDrawArrays(GL_LINES, 0, vcount / 3);
		}

		if (cols)
			glDisableVertexAttribArray(VertexColor);

		glDisableVertexAttribArray(VertexPosition);
	}
}

bool RenderInputScene::FrustumCull(const Drawable & d) const
{
	const float radius = d.GetRadius();
	if (radius > 0.0)
	{
		// get object center in world space
		Vec3 objpos = d.GetObjectCenter();
		d.GetTransform().TransformVectorOut(objpos[0], objpos[1], objpos[2]);

		// get distance to camera
		const float dx = objpos[0] - cam_position[0];
		const float dy = objpos[1] - cam_position[1];
		const float dz = objpos[2] - cam_position[2];
		const float rc = dx * dx + dy * dy + dz * dz;

		// test against camera position (assuming near plane is zero)
		if (rc < radius * radius)
			return false;

		// test against camera far plane
		const float temp_lod_far = lod_far + radius;
		if (rc > temp_lod_far * temp_lod_far)
			return true;

		// test against all frustum planes
		for (int i = 0; i < 6; i++)
		{
			const float rd = frustum.frustum[i][0] * objpos[0] +
				frustum.frustum[i][1] * objpos[1] +
				frustum.frustum[i][2] * objpos[2] +
				frustum.frustum[i][3];
			if (rd <= -radius)
				return true;
		}
	}
	return false;
}

void RenderInputScene::SetFlags(const Drawable & d, GraphicsState & glstate)
{
	glstate.DepthOffset(d.GetDecal());

	if (d.GetCull())
	{
		glstate.CullFace(true);
		if (d.GetCullFront())
			glstate.CullFaceMode(GL_FRONT);
		else
			glstate.CullFaceMode(GL_BACK);
	}
	else
	{
		glstate.CullFace(false);
	}

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

void RenderInputScene::SetTransform(const Drawable & d, GraphicsState & glstate)
{
	if (!last_transform_valid || !last_transform.Equals(d.GetTransform()))
	{
		Mat4 worldTrans = d.GetTransform().Multiply(viewMatrix);
		glLoadMatrixf(worldTrans.GetArray());
		last_transform = d.GetTransform();
		last_transform_valid = true;
	}
}
