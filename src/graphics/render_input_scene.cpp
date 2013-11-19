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
#include "glutil.h"
#include "graphicsstate.h"
#include "drawable.h"
#include "texture.h"
#include "shader.h"
#include "vertexarray.h"

RenderInputScene::RenderInputScene():
	last_transform_valid(false),
	shaders(false),
	clearcolor(false),
	cleardepth(false),
	orthomode(false),
	contrast(1.0),
	depth_mode(GL_LEQUAL),
	writecolor(true),
	writedepth(true),
	carpainthack(false),
	vlighting(false),
	blendmode(BlendMode::DISABLED)
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

void RenderInputScene::SetDrawLists(
	const std::vector <Drawable*> & dl_dynamic,
	const std::vector <Drawable*> & dl_static)
{
	dynamic_drawlist_ptr = &dl_dynamic;
	static_drawlist_ptr = &dl_static;
}

void RenderInputScene::DisableOrtho()
{
	orthomode = false;
}

void RenderInputScene::SetOrtho(
	const Vec3 & neworthomin,
	const Vec3 & neworthomax)
{
	orthomode = true;
	orthomin = neworthomin;
	orthomax = neworthomax;
}

Frustum RenderInputScene::SetCameraInfo(
	const Vec3 & newpos,
	const Quat & newrot,
	float newfov, float newlodfar,
	float neww, float newh,
	bool restore_matrices)
{
	cam_position = newpos;
	cam_rotation = newrot;
	camfov = newfov;
	lod_far = newlodfar;
	w = neww;
	h = newh;

	if (orthomode)
		projMatrix.SetOrthographic(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
	else
		projMatrix.Perspective(camfov, w/(float)h, 0.1f, lod_far);

	cam_rotation.GetMatrix4(viewMatrix);
	float translate[4] = {-cam_position[0], -cam_position[1], -cam_position[2], 0};
	viewMatrix.MultiplyVector4(translate);
	viewMatrix.Translate(translate[0], translate[1], translate[2]);

	frustum.Extract(projMatrix.GetArray(), viewMatrix.GetArray());
	return frustum;
}

const Mat4 & RenderInputScene::GetProjMatrix() const
{
	return projMatrix;
}

const Mat4 & RenderInputScene::GetViewMatrix() const
{
	return viewMatrix;
}

void RenderInputScene::SetSunDirection(const Vec3 & newsun)
{
	lightposition = newsun;
}

void RenderInputScene::SetFlags(bool newshaders)
{
	shaders = newshaders;
}

void RenderInputScene::SetDefaultShader(Shader & newdefault)
{
	assert(newdefault.GetLoaded());
	shader = newdefault;
}

void RenderInputScene::SetClear(bool newclearcolor, bool newcleardepth)
{
	clearcolor = newclearcolor;
	cleardepth = newcleardepth;
}

void RenderInputScene::Render(GraphicsState & glstate, std::ostream & error_output)
{
	if (orthomode)
		projMatrix.SetOrthographic(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
	else
		projMatrix.Perspective(camfov, w/(float)h, 0.1f, lod_far);

	cam_rotation.GetMatrix4(viewMatrix);
	float translate[4] = {-cam_position[0], -cam_position[1], -cam_position[2], 0};
	viewMatrix.MultiplyVector4(translate);
	viewMatrix.Translate(translate[0], translate[1], translate[2]);

	frustum.Extract(projMatrix.GetArray(), viewMatrix.GetArray());

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projMatrix.GetArray());

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(viewMatrix.GetArray());

	// send information to the shaders
	if (shaders)
	{
		assert(shader);

		// cubemap transform goes in texture2
		Quat camlook;
		camlook.Rotate(M_PI_2, 1, 0, 0);
		camlook.Rotate(-M_PI_2, 0, 0, 1);
		Quat cuberotation;
		cuberotation = (-camlook) * (-cam_rotation); // experimentally derived
		Mat4 cubeMatrix;
		cuberotation.GetMatrix4(cubeMatrix);

		glActiveTexture(GL_TEXTURE2);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(cubeMatrix.GetArray());
		glActiveTexture(GL_TEXTURE0);
		glMatrixMode(GL_MODELVIEW);

		// send light position to the shaders
		Vec3 lightvec = lightposition;
		(cam_rotation).RotateVector(lightvec);
		shader->Enable();
		shader->UploadActiveShaderParameter3f("lightposition", lightvec[0], lightvec[1], lightvec[2]);
		shader->UploadActiveShaderParameter1f("contrast", contrast);
	}
	else
	{
		// carpainthack is only used with dynamic objects(cars)
		if (carpainthack && !dynamic_drawlist_ptr->empty())
			EnableCarPaint(glstate);
		else
			DisableCarPaint(glstate);
	}

	glstate.SetColorMask(writecolor, writealpha);
	glstate.SetDepthMask(writedepth);

	glstate.ClearDrawBuffer(clearcolor, cleardepth);

	if (writedepth || depth_mode != GL_ALWAYS)
		glstate.Enable(GL_DEPTH_TEST);
	else
		glstate.Disable(GL_DEPTH_TEST);

	glDepthFunc(depth_mode);

	SetBlendMode(glstate);

	glstate.SetColor(1,1,1,1);

	last_transform_valid = false;

	DrawList(glstate, *dynamic_drawlist_ptr, false);
	DrawList(glstate, *static_drawlist_ptr, true);
}

void RenderInputScene::SetFSAA(unsigned int value)
{
	fsaa = value;
}

void RenderInputScene::SetContrast(float value)
{
	contrast = value;
}

void RenderInputScene::SetDepthMode(int mode)
{
	depth_mode = mode;
}

void RenderInputScene::SetWriteDepth(bool write)
{
	writedepth = write;
}

void RenderInputScene::SetWriteColor(bool write)
{
	writecolor = write;
}

void RenderInputScene::SetWriteAlpha(bool write)
{
	writealpha = write;
}

std::pair <bool, bool> RenderInputScene::GetClear() const
{
	return std::make_pair(clearcolor, cleardepth);
}

void RenderInputScene::SetCarPaintHack(bool hack)
{
	carpainthack = hack;
}

void RenderInputScene::SetBlendMode(BlendMode::BLENDMODE mode)
{
	blendmode = mode;
}

void RenderInputScene::EnableCarPaint(GraphicsState & glstate)
{
	// turn on lighting for cars only atm
	if (!vlighting)
	{
		Vec3 lightvec = lightposition;
		cam_rotation.RotateVector(lightvec);

		// push some sane values, should be configurable maybe?
		// vcol = light_ambient * material_ambient
		// vcol += L.N * light_diffuse * material_diffuse
		// vcol += (H.N)^n * light_specular * material_specular
		GLfloat pos[] = {lightvec[0], lightvec[1], lightvec[2], 0.0f};
		GLfloat diffuse[] = {0.4f, 0.4f, 0.4f, 1.0f};
		GLfloat ambient[] = {0.6f, 0.6f, 0.6f, 1.0f};
		glEnable(GL_LIGHTING);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glEnable(GL_LIGHT0);

		GLfloat mcolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mcolor);

		vlighting = true;

		// dummy texture required to set the combiner
		Drawable & d = *dynamic_drawlist_ptr->front();

		// setup first combiner
		glstate.BindTexture2D(0, d.GetTexture0());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
		// don't care about alpha, set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		// setup second combiner explicitly
		// statemanager doesnt allow to enable/disable textures per tu
		//glstate.BindTexture2D(1, d.GetTexture0());
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, d.GetTexture0());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		// don't care about alpha, set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glActiveTexture(GL_TEXTURE0);
	}
}

void RenderInputScene::DisableCarPaint(GraphicsState & glstate)
{
	if (vlighting)
	{
		// turn off lighting for everything else
		glDisable(GL_LIGHTING);

		// reset second texture combiner explicitly
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);

		// reset first texture combiner
		glstate.Disable(GL_TEXTURE_2D);

		vlighting = false;
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void RenderInputScene::SetBlendMode(GraphicsState & glstate)
{
	switch (blendmode)
	{
		case BlendMode::DISABLED:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Disable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		}
		break;

		case BlendMode::ADD:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		break;

		case BlendMode::ALPHABLEND:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		case BlendMode::PREMULTIPLIED_ALPHA:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		case BlendMode::ALPHATEST:
		{
			glstate.Enable(GL_ALPHA_TEST);
			glstate.Disable(GL_BLEND);
			if (fsaa > 1 && shaders)
			{
				glstate.Enable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			}
			glstate.SetAlphaFunc(GL_GREATER, 0.5f);
		}
		break;

		default:
		assert(0);
		break;
	}
}

void RenderInputScene::DrawList(GraphicsState & glstate, const std::vector <Drawable*> & drawlist, bool preculled)
{
	for (std::vector <Drawable*>::const_iterator ptr = drawlist.begin(); ptr != drawlist.end(); ++ptr)
	{
		const Drawable & d = **ptr;
		if (preculled || !FrustumCull(d))
		{
			SetFlags(d, glstate);

			SetTextures(d, glstate);

			SetTransform(d, glstate);

			if (d.GetDrawList())
			{
				glCallList(d.GetDrawList());
			}
			else if (d.GetVertArray())
			{
				DrawVertexArray(*d.GetVertArray(), d.GetLineSize());
			}
		}
	}
}

void RenderInputScene::DrawVertexArray(const VertexArray & va, float linesize) const
{
	const float * verts;
	int vertcount;
	va.GetVertices(verts, vertcount);
	if (verts)
	{
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glEnableClientState(GL_VERTEX_ARRAY);

		const unsigned char * cols;
		int colcount;
		va.GetColors(cols, colcount);
		if (cols)
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, cols);
			glEnableClientState(GL_COLOR_ARRAY);
		}

		const int * faces;
		int facecount;
		va.GetFaces(faces, facecount);
		if (faces)
		{
			const float * norms;
			int normcount;
			va.GetNormals(norms, normcount);
			if (norms)
			{
				glNormalPointer(GL_FLOAT, 0, norms);
				glEnableClientState(GL_NORMAL_ARRAY);
			}

			const float * tc = 0;
			int tccount;
			if (va.GetTexCoordSets() > 0)
			{
				va.GetTexCoords(0, tc, tccount);
				if (tc)
				{
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0, tc);
				}
			}

			glDrawElements(GL_TRIANGLES, facecount, GL_UNSIGNED_INT, faces);

			if (tc)
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			if (norms)
				glDisableClientState(GL_NORMAL_ARRAY);
		}
		else if (linesize > 0)
		{
			glLineWidth(linesize);
			glDrawArrays(GL_LINES, 0, vertcount / 3);
		}

		if (cols)
			glDisableClientState(GL_COLOR_ARRAY);

		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

bool RenderInputScene::FrustumCull(const Drawable & d)
{
	if (d.GetRadius() > 0.0)
	{
		//do frustum culling
		Vec3 objpos(d.GetObjectCenter());
		d.GetTransform().TransformVectorOut(objpos[0],objpos[1],objpos[2]);
		float dx=objpos[0]-cam_position[0]; float dy=objpos[1]-cam_position[1]; float dz=objpos[2]-cam_position[2];
		float rc=dx*dx+dy*dy+dz*dz;
		float temp_lod_far = lod_far + d.GetRadius();
		if (rc > temp_lod_far*temp_lod_far)
			return true;
		else if (rc < d.GetRadius()*d.GetRadius())
			return false;
		else
		{
			float bound, rd;
			bound = d.GetRadius();
			for (int i=0; i<6; i++)
			{
				rd=frustum.frustum[i][0]*objpos[0]+
						frustum.frustum[i][1]*objpos[1]+
						frustum.frustum[i][2]*objpos[2]+
						frustum.frustum[i][3];
				if (rd <= -bound)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void RenderInputScene::SetFlags(const Drawable & d, GraphicsState & glstate)
{
	if (d.GetDecal())
	{
		glstate.Enable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glstate.Disable(GL_POLYGON_OFFSET_FILL);
	}

	if (d.GetCull())
	{
		glstate.Enable(GL_CULL_FACE);
		if (d.GetCullFront())
			glstate.SetCullFace(GL_FRONT);
		else
			glstate.SetCullFace(GL_BACK);
	}
	else
	{
		glstate.Disable(GL_CULL_FACE);
	}

	const Vec4 & color = d.GetColor();
	glstate.SetColor(color[0], color[1], color[2], color[3]);
}

void RenderInputScene::SetTextures(const Drawable & d, GraphicsState & glstate)
{
	if (!d.GetTexture0())
	{
		glstate.Disable(GL_TEXTURE_2D);
		return;
	}

	glstate.BindTexture2D(0, d.GetTexture0());

	if (carpainthack)
	{
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &d.GetColor()[0]);
	}

	if (shaders)
	{
		glstate.BindTexture2D(1, d.GetTexture1());
		glstate.BindTexture2D(2, d.GetTexture2());
	}
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
