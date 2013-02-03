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
#include "glstatemanager.h"
#include "drawable.h"
#include "texture.h"
#include "shader.h"

RENDER_INPUT_SCENE::RENDER_INPUT_SCENE():
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
	blendmode(BLENDMODE::DISABLED)
{
	MATHVECTOR <float, 3> front(1,0,0);
	lightposition = front;
	QUATERNION <float> ldir;
	ldir.Rotate(3.141593*0.5,1,0,0);
	ldir.RotateVector(lightposition);
}

RENDER_INPUT_SCENE::~RENDER_INPUT_SCENE()
{
	//dtor
}

void RENDER_INPUT_SCENE::SetDrawLists(
	const std::vector <DRAWABLE*> & dl_dynamic,
	const std::vector <DRAWABLE*> & dl_static)
{
	dynamic_drawlist_ptr = &dl_dynamic;
	static_drawlist_ptr = &dl_static;
}

void RENDER_INPUT_SCENE::DisableOrtho()
{
	orthomode = false;
}

void RENDER_INPUT_SCENE::SetOrtho(
	const MATHVECTOR <float, 3> & neworthomin,
	const MATHVECTOR <float, 3> & neworthomax)
{
	orthomode = true;
	orthomin = neworthomin;
	orthomax = neworthomax;
}

FRUSTUM RENDER_INPUT_SCENE::SetCameraInfo(
	const MATHVECTOR <float, 3> & newpos,
	const QUATERNION <float> & newrot,
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

const MATRIX4<float> & RENDER_INPUT_SCENE::GetProjMatrix() const
{
	return projMatrix;
}

const MATRIX4<float> & RENDER_INPUT_SCENE::GetViewMatrix() const
{
	return viewMatrix;
}

void RENDER_INPUT_SCENE::SetSunDirection(const MATHVECTOR <float, 3> & newsun)
{
	lightposition = newsun;
}

void RENDER_INPUT_SCENE::SetFlags(bool newshaders)
{
	shaders = newshaders;
}

void RENDER_INPUT_SCENE::SetDefaultShader(SHADER_GLSL & newdefault)
{
	assert(newdefault.GetLoaded());
	shader = newdefault;
}

void RENDER_INPUT_SCENE::SetClear(bool newclearcolor, bool newcleardepth)
{
	clearcolor = newclearcolor;
	cleardepth = newcleardepth;
}

void RENDER_INPUT_SCENE::Render(GLSTATEMANAGER & glstate, std::ostream & error_output)
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

		// camera transform goes in texture3
		glActiveTexture(GL_TEXTURE3);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(viewMatrix.GetArray());

		// cubemap transform goes in texture2
		QUATERNION <float> camlook;
		camlook.Rotate(M_PI_2, 1, 0, 0);
		camlook.Rotate(-M_PI_2, 0, 0, 1);
		QUATERNION <float> cuberotation;
		cuberotation = (-camlook) * (-cam_rotation); // experimentally derived
		MATRIX4<float> cubeMatrix;
		cuberotation.GetMatrix4(cubeMatrix);

		glActiveTexture(GL_TEXTURE2);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(cubeMatrix.GetArray());

		glActiveTexture(GL_TEXTURE0);
		glMatrixMode(GL_MODELVIEW);

		// send light position to the shaders
		MATHVECTOR <float, 3> lightvec = lightposition;
		(cam_rotation).RotateVector(lightvec);
		shader->Enable();
		shader->UploadActiveShaderParameter3f("lightposition", lightvec[0], lightvec[1], lightvec[2]);
		shader->UploadActiveShaderParameter1f("contrast", contrast);

		glActiveTexture(GL_TEXTURE0);

		PushShadowMatrices();
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

	if (clearcolor && cleardepth)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if (clearcolor)
		glClear(GL_COLOR_BUFFER_BIT);
	else if (cleardepth)
		glClear(GL_DEPTH_BUFFER_BIT);

	if (writedepth || depth_mode != GL_ALWAYS)
		glstate.Enable(GL_DEPTH_TEST);
	else
		glstate.Disable(GL_DEPTH_TEST);

	glDepthFunc(depth_mode);

	SetBlendMode(glstate);

	last_transform_valid = false;

	glColor4f(1,1,1,1);
	glstate.SetColor(1,1,1,1);

	DrawList(glstate, *dynamic_drawlist_ptr, false);
	DrawList(glstate, *static_drawlist_ptr, true);

	if (last_transform_valid)
		glPopMatrix();

	if (shaders)
		PopShadowMatrices();
}

void RENDER_INPUT_SCENE::SetReflection(TEXTURE_INTERFACE * value)
{
	if (!value)
		reflection.clear();
	else
		reflection = value;
}

void RENDER_INPUT_SCENE::SetFSAA(unsigned int value)
{
	fsaa = value;
}

void RENDER_INPUT_SCENE::SetAmbient(TEXTURE_INTERFACE & value)
{
	ambient = value;
}

void RENDER_INPUT_SCENE::SetContrast(float value)
{
	contrast = value;
}

void RENDER_INPUT_SCENE::SetDepthMode(int mode)
{
	depth_mode = mode;
}

void RENDER_INPUT_SCENE::SetWriteDepth(bool write)
{
	writedepth = write;
}

void RENDER_INPUT_SCENE::SetWriteColor(bool write)
{
	writecolor = write;
}

void RENDER_INPUT_SCENE::SetWriteAlpha(bool write)
{
	writealpha = write;
}

std::pair <bool, bool> RENDER_INPUT_SCENE::GetClear() const
{
	return std::make_pair(clearcolor, cleardepth);
}

void RENDER_INPUT_SCENE::SetCarPaintHack(bool hack)
{
	carpainthack = hack;
}

void RENDER_INPUT_SCENE::SetBlendMode(BLENDMODE::BLENDMODE mode)
{
	blendmode = mode;
}

void RENDER_INPUT_SCENE::EnableCarPaint(GLSTATEMANAGER & glstate)
{
	// turn on lighting for cars only atm
	if (!vlighting)
	{
		MATHVECTOR <float, 3> lightvec = lightposition;
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
		DRAWABLE & d = *dynamic_drawlist_ptr->front();

		// setup first combiner
		glstate.BindTexture2D(0, d.GetDiffuseMap());
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
		//glstate.BindTexture2D(1, d.GetDiffuseMap());
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, d.GetDiffuseMap()->GetID());
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

void RENDER_INPUT_SCENE::DisableCarPaint(GLSTATEMANAGER & glstate)
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

void RENDER_INPUT_SCENE::SetBlendMode(GLSTATEMANAGER & glstate)
{
	switch (blendmode)
	{
		case BLENDMODE::DISABLED:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Disable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		}
		break;

		case BLENDMODE::ADD:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		break;

		case BLENDMODE::ALPHABLEND:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		case BLENDMODE::PREMULTIPLIED_ALPHA:
		{
			glstate.Disable(GL_ALPHA_TEST);
			glstate.Enable(GL_BLEND);
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		break;

		case BLENDMODE::ALPHATEST:
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

void RENDER_INPUT_SCENE::DrawList(GLSTATEMANAGER & glstate, const std::vector <DRAWABLE*> & drawlist, bool preculled)
{
	for (std::vector <DRAWABLE*>::const_iterator ptr = drawlist.begin(); ptr != drawlist.end(); ++ptr)
	{
		const DRAWABLE & d = **ptr;
		if (preculled || !FrustumCull(d))
		{
			SetFlags(d, glstate);

			SetTextures(d, glstate);

			bool need_pop = SetTransform(d, glstate);

			if (d.IsDrawList())
			{
				const unsigned int numlists = d.GetDrawLists().size();
				for (unsigned int n = 0; n < numlists; ++n)
					glCallList(d.GetDrawLists()[n]);
			}
			else if (d.GetVertArray())
			{
				DrawVertexArray(*d.GetVertArray(), d.GetLineSize());
			}

			if (need_pop)
				glPopMatrix();
		}
	}
}

void RENDER_INPUT_SCENE::DrawVertexArray(const VERTEXARRAY & va, float linesize) const
{
	const float * verts;
	int vertcount;
	va.GetVertices(verts, vertcount);
	if (vertcount > 0 && verts)
	{
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glEnableClientState(GL_VERTEX_ARRAY);

		const int * faces;
		int facecount;
		va.GetFaces(faces, facecount);
		if (facecount > 0 && faces)
		{
			const float * norms;
			int normcount;
			va.GetNormals(norms, normcount);
			if (normcount > 0 && norms)
			{
				glNormalPointer(GL_FLOAT, 0, norms);
				glEnableClientState(GL_NORMAL_ARRAY);
			}

			const float * tc[1];
			int tccount[1];
			if (va.GetTexCoordSets() > 0)
			{
				va.GetTexCoords(0, tc[0], tccount[0]);
				if (tc[0] && tccount[0])
				{
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glTexCoordPointer(2, GL_FLOAT, 0, tc[0]);
				}
			}

			glDrawElements(GL_TRIANGLES, facecount, GL_UNSIGNED_INT, faces);

			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		else if (linesize > 0)
		{
			glLineWidth(linesize);
			glDrawArrays(GL_LINES, 0, vertcount/3);
		}
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

bool RENDER_INPUT_SCENE::FrustumCull(const DRAWABLE & d)
{
	if (d.GetRadius() != 0.0 && !d.GetSkybox() && d.GetCameraTransformEnable())
	{
		//do frustum culling
		MATHVECTOR <float, 3> objpos(d.GetObjectCenter());
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

void RENDER_INPUT_SCENE::SetFlags(const DRAWABLE & d, GLSTATEMANAGER & glstate)
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

	float r,g,b,a;
	d.GetColor(r,g,b,a);
	glstate.SetColor(r,g,b,a);
}

void RENDER_INPUT_SCENE::SetTextures(const DRAWABLE & d, GLSTATEMANAGER & glstate)
{
	const TEXTURE * diffusetexture = d.GetDiffuseMap();

	if (!diffusetexture || !diffusetexture->Loaded())
	{
		glstate.Disable(GL_TEXTURE_2D);
		return;
	}

	glstate.BindTexture2D(0, d.GetDiffuseMap());

	if (carpainthack)
	{
		GLfloat color[4] = {0.0, 0.0, 0.0, 1.0};
		d.GetColor(color[0], color[1], color[2], color[3]);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
	}

	if (shaders)
	{
		glstate.BindTexture2D(1, d.GetMiscMap1());
		glstate.BindTexture2D(2, d.GetMiscMap2());
	}
}

bool RENDER_INPUT_SCENE::SetTransform(const DRAWABLE & d, GLSTATEMANAGER & glstate)
{
	bool need_a_pop = true;
	if (!d.GetCameraTransformEnable()) //do our own transform only and ignore the camera position / orientation
	{
		if (last_transform_valid)
			glPopMatrix();
		last_transform_valid = false;

		glPushMatrix();
		glLoadMatrixf(d.GetTransform().GetArray());
	}
	else if (d.GetSkybox())
	{
		if (last_transform_valid)
			glPopMatrix();
		last_transform_valid = false;

		glPushMatrix();
		float temp_matrix[16];
		cam_rotation.GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		if (d.GetVerticalTrack())
		{
			MATHVECTOR< float, 3 > objpos(d.GetObjectCenter());
			d.GetTransform().TransformVectorOut(objpos[0], objpos[1], objpos[2]);
			glTranslatef(0.0, 0.0, -objpos[2]);
		}
		glMultMatrixf(d.GetTransform().GetArray());
	}
	else
	{
		bool need_new_transform = !last_transform_valid;
		if (last_transform_valid)
			need_new_transform = (!last_transform.Equals(d.GetTransform()));
		if (need_new_transform)
		{
			if (last_transform_valid)
				glPopMatrix();

			glPushMatrix();
			glMultMatrixf(d.GetTransform().GetArray());
			last_transform = d.GetTransform();
			last_transform_valid = true;
		}
		need_a_pop = false;
	}
	return need_a_pop;
}

/*unsigned int GRAPHICS_SDLGL::RENDER_INPUT_SCENE::CombineDrawlists()
{
	combined_drawlist_cache.resize(0);
	combined_drawlist_cache.reserve(drawlist_static->size()+drawlist_dynamic->size());

	unsigned int already_culled = 0;

	if (use_static_partitioning)
	{
		AABB<float>::FRUSTUM aabbfrustum(frustum);
		//aabbfrustum.DebugPrint(std::cout);
		static_partitioning->Query(aabbfrustum, combined_drawlist_cache);
		already_culled = combined_drawlist_cache.size();
	}
	else
		calgo::transform(*drawlist_static, std::back_inserter(combined_drawlist_cache), &PointerTo);
	calgo::transform(*drawlist_dynamic, std::back_inserter(combined_drawlist_cache), &PointerTo);

	return already_culled;
}*/
