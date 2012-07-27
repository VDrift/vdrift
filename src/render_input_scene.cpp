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

	glMatrixMode( GL_PROJECTION );
	if (restore_matrices)
		glPushMatrix();
	glLoadIdentity();
	if (orthomode)
	{
		glOrtho(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
	}
	else
	{
		gluPerspective( camfov, w/(float)h, 0.1f, lod_far );
	}
	glMatrixMode( GL_MODELVIEW );
	if (restore_matrices)
		glPushMatrix();
	float temp_matrix[16];
	(cam_rotation).GetMatrix4(temp_matrix);
	glLoadMatrixf(temp_matrix);
	glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);
	ExtractFrustum(frustum);
	glMatrixMode( GL_PROJECTION );
	if (restore_matrices)
		glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	if (restore_matrices)
		glPopMatrix();
	return frustum;
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
	if (shaders)
		assert(shader);

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	if (orthomode)
	{
		glOrtho(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
		//std::cout << "ortho near/far: " << orthomin[2] << "/" << orthomax[2] << std::endl;
	}
	else
	{
		gluPerspective( camfov, w/(float)h, 0.1f, lod_far );
	}
	glMatrixMode( GL_MODELVIEW );
	float temp_matrix[16];
	(cam_rotation).GetMatrix4(temp_matrix);
	glLoadMatrixf(temp_matrix);
	glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);
	ExtractFrustum(frustum);

	//send information to the shaders
	if (shaders)
	{
		//camera transform goes in texture3
		glActiveTexture(GL_TEXTURE3);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		(cam_rotation).GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);

		//cubemap transform goes in texture2
		glActiveTexture(GL_TEXTURE2);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5,1,0,0);
		camlook.Rotate(-3.141593*0.5,0,0,1);
		QUATERNION <float> cuberotation;
		cuberotation = (-camlook) * (-cam_rotation); //experimentally derived
		(cuberotation).GetMatrix4(temp_matrix);
		//(cam_rotation).GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		//glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);
		//glLoadIdentity();

		glActiveTexture(GL_TEXTURE0);
		glMatrixMode(GL_MODELVIEW);

		//send light position to the shaders
		MATHVECTOR <float, 3> lightvec = lightposition;
		(cam_rotation).RotateVector(lightvec);
		//(cuberotation).RotateVector(lightvec);
		shader->Enable();
		shader->UploadActiveShaderParameter3f("lightposition", lightvec[0], lightvec[1], lightvec[2]);
		shader->UploadActiveShaderParameter1f("contrast", contrast);
		/*float lightarray[3];
		for (int i = 0; i < 3; i++)
		lightarray[i] = lightvec[i];
		glLightfv(GL_LIGHT0, GL_POSITION, lightarray);*/

		// if we have no reflection texture supplied, don't touch the TU because
		// someone else may be supplying one
		/*if (reflection && reflection->Loaded())
		{
			glActiveTexture(GL_TEXTURE2);
			reflection->Activate();
			glActiveTexture(GL_TEXTURE0);
		}*/

		/*glActiveTexture(GL_TEXTURE3);
		if (ambient && ambient->Loaded())
		{
			ambient->Activate();
		}
		else
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP,0);
			//assert(0);
		}*/
		glActiveTexture(GL_TEXTURE0);

		PushShadowMatrices();
	}
	else
	{
		// carpainthack is only used with dynamic objects(cars)
		if (carpainthack && !dynamic_drawlist_ptr->empty())
		{
			// turn on lighting for cars only atm
			if (!vlighting)
			{
				MATHVECTOR <float, 3> lightvec = lightposition;
				(cam_rotation).RotateVector(lightvec);

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

				// setup texture combiners here

				// need some dummy texture
				DRAWABLE & d = *dynamic_drawlist_ptr->front();

				// setup first combiner
				//glstate.BindTexture2D(0, d.GetDiffuseMap()); //fails???
				glActiveTexture(GL_TEXTURE0);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, d.GetDiffuseMap()->GetID());
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
				// don't care about alpha; set it to something harmless
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_CONSTANT);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

				// setup second combiner
				//glstate.BindTexture2D(1, d.GetDiffuseMap()); //fails???
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, d.GetDiffuseMap()->GetID());
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
				// don't care about alpha; set it to something harmless
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			}
		}
		else
		{
			if (vlighting)
			{
				// turn off lighting for everything else
				glDisable(GL_LIGHTING);

				// reset first texture combiner
				glActiveTexture(GL_TEXTURE0);
				glDisable(GL_TEXTURE_2D);

				// reset second texture combiner
				glActiveTexture(GL_TEXTURE1);
				glDisable(GL_TEXTURE_2D);

				vlighting = false;
			}

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}

	//std::cout << "scene: " << std::endl;

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

	glDepthFunc( depth_mode );

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

void RENDER_INPUT_SCENE::DrawList(GLSTATEMANAGER & glstate, const std::vector <DRAWABLE*> & drawlist, bool preculled)
{
	unsigned int drawcount = 0;
	unsigned int loopcount = 0;

	for (std::vector <DRAWABLE*>::const_iterator ptr = drawlist.begin(); ptr != drawlist.end(); ptr++, loopcount++)
	{
		DRAWABLE * i = *ptr;
		if (preculled || !FrustumCull(*i))
		{
			drawcount++;

			SelectFlags(*i, glstate);

			if (shaders) SelectAppropriateShader(*i);

			SelectTexturing(*i, glstate);

			bool need_pop = SelectTransformStart(*i, glstate);

			//assert(i->GetDraw()->GetVertArray() || i->GetDraw()->IsDrawList() || !i->GetDraw()->GetLine().empty());

			if (i->IsDrawList())
			{
				const unsigned int numlists = i->GetDrawLists().size();
				for (unsigned int n = 0; n < numlists; ++n)
					glCallList(i->GetDrawLists()[n]);
			}
			else if (i->GetVertArray())
			{
				const float * verts;
				int vertcount;
				i->GetVertArray()->GetVertices(verts, vertcount);
				if (vertcount > 0 && verts)
				{
					glVertexPointer(3, GL_FLOAT, 0, verts);
					glEnableClientState(GL_VERTEX_ARRAY);

					const int * faces;
					int facecount;
					i->GetVertArray()->GetFaces(faces, facecount);
					if (facecount > 0 && faces)
					{
						const float * norms;
						int normcount;
						i->GetVertArray()->GetNormals(norms, normcount);
						if (normcount > 0 && norms)
						{
							glNormalPointer(GL_FLOAT, 0, norms);
							glEnableClientState(GL_NORMAL_ARRAY);
						}

						const float * tc[1];
						int tccount[1];
						if (i->GetVertArray()->GetTexCoordSets() > 0)
						{
							i->GetVertArray()->GetTexCoords(0, tc[0], tccount[0]);
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
					else if (i->GetLineSize() > 0)
					{
						glstate.Enable(GL_LINE_SMOOTH);
						glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
						glLineWidth(i->GetLineSize());
						glDrawArrays(GL_LINES,  0, vertcount/3);
					}
					glDisableClientState(GL_VERTEX_ARRAY);
				}
			}
			SelectTransformEnd(*i, need_pop);
		}
	}
}

bool RENDER_INPUT_SCENE::FrustumCull(DRAWABLE & tocull)
{
	//return false;

	DRAWABLE * d (&tocull);
	//if (d->GetRadius() != 0.0 && d->parent != NULL && !d->skybox)
	if (d->GetRadius() != 0.0 && !d->GetSkybox() && d->GetCameraTransformEnable())
	{
		//do frustum culling
		MATHVECTOR <float, 3> objpos(d->GetObjectCenter());
		d->GetTransform().TransformVectorOut(objpos[0],objpos[1],objpos[2]);
		float dx=objpos[0]-cam_position[0]; float dy=objpos[1]-cam_position[1]; float dz=objpos[2]-cam_position[2];
		float rc=dx*dx+dy*dy+dz*dz;
		float temp_lod_far = lod_far + d->GetRadius();
		if (rc > temp_lod_far*temp_lod_far)
			return true;
		else if (rc < d->GetRadius()*d->GetRadius())
			return false;
		else
		{
			float bound, rd;
			bound = d->GetRadius();
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

void RENDER_INPUT_SCENE::SelectAppropriateShader(DRAWABLE & forme)
{
	(void)forme;
	// deprecated! put the appropriate shader for the drawable group in your render.conf
}

void RENDER_INPUT_SCENE::SelectFlags(DRAWABLE & forme, GLSTATEMANAGER & glstate)
{
	DRAWABLE * i(&forme);
	if (i->GetDecal())
	{
		glstate.Enable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glstate.Disable(GL_POLYGON_OFFSET_FILL);
	}

	if (i->GetCull())
	{
		glstate.Enable(GL_CULL_FACE);
		if (i->GetCull())
		{
			if (i->GetCullFront())
				glstate.SetCullFace(GL_FRONT);
			else
				glstate.SetCullFace(GL_BACK);
		}
	}
	else
		glstate.Disable(GL_CULL_FACE);

	float r,g,b,a;
	i->GetColor(r,g,b,a);
	glstate.SetColor(r,g,b,a);
}

void RENDER_INPUT_SCENE::SelectTexturing(DRAWABLE & forme, GLSTATEMANAGER & glstate)
{
	DRAWABLE * i(&forme);

	bool enabletex = true;

	const TEXTURE * diffusetexture = i->GetDiffuseMap();

	if (!diffusetexture)
		enabletex = false;
	else if (!diffusetexture->Loaded())
		enabletex = false;

	if (!enabletex)
	{
		glstate.Disable(GL_TEXTURE_2D);
		return;
	}

	glstate.BindTexture2D(0, i->GetDiffuseMap());

	if (carpainthack)
	{
		GLfloat color[4] = {0.0, 0.0, 0.0, 1.0};
		forme.GetColor(color[0], color[1], color[2], color[3]);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
	}

	if (shaders)
	{
		glstate.BindTexture2D(1, i->GetMiscMap1());
		glstate.BindTexture2D(2, i->GetMiscMap2());
	}
}

///returns true if the matrix was pushed
bool RENDER_INPUT_SCENE::SelectTransformStart(DRAWABLE & forme, GLSTATEMANAGER & glstate)
{
	bool need_a_pop = true;

	DRAWABLE * i(&forme);
	if (!i->GetCameraTransformEnable()) //do our own transform only and ignore the camera position / orientation
	{
		if (last_transform_valid)
			glPopMatrix();
		last_transform_valid = false;

		glPushMatrix();
		glLoadMatrixf(i->GetTransform().GetArray());
	}
	else if (i->GetSkybox())
	{
		if (last_transform_valid)
			glPopMatrix();
		last_transform_valid = false;

		glPushMatrix();
		float temp_matrix[16];
		cam_rotation.GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		if (i->GetVerticalTrack())
		{
			MATHVECTOR< float, 3 > objpos(i->GetObjectCenter());
			//std::cout << "Vertical offset: " << objpos;
			i->GetTransform().TransformVectorOut(objpos[0],objpos[1],objpos[2]);
			//std::cout << " || " << objpos << endl;
			//glTranslatef(-objpos.x,-objpos.y,-objpos.z);
			//glTranslatef(0,game.cam.position.y,0);
			glTranslatef(0.0,0.0,-objpos[2]);
		}
		glMultMatrixf(i->GetTransform().GetArray());
	}
	else
	{
		bool need_new_transform = !last_transform_valid;
		if (last_transform_valid)
			need_new_transform = (!last_transform.Equals(i->GetTransform()));
		if (need_new_transform)
		{
			if (last_transform_valid)
				glPopMatrix();

			glPushMatrix();
			glMultMatrixf(i->GetTransform().GetArray());
			last_transform = i->GetTransform();
			last_transform_valid = true;

			need_a_pop = false;
		}
		else need_a_pop = false;
	}

	// throw information about the object into the texture 1 matrix
	/*glActiveTexture(GL_TEXTURE1);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	static float temp_matrix[16];
	for (int n = 0; n < 3; n++)
		temp_matrix[n] = i->GetObjectCenter()[n];
	temp_matrix[3] = 1.0;
	MATRIX4<float> m;
	glGetFloatv (GL_MODELVIEW_MATRIX, m.GetArray());
	m.MultiplyVector4(temp_matrix); //eyespace light center in 0, 1, 2
	temp_matrix[3] = 0.1; //attenuation factor in 3
	glLoadMatrixf(temp_matrix);
	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_MODELVIEW);*/

	return need_a_pop;
}

void RENDER_INPUT_SCENE::SelectTransformEnd(DRAWABLE & forme, bool need_pop)
{
	if (need_pop)
	{
		glPopMatrix();
	}
}

/*SCENEDRAW * PointerTo(const SCENEDRAW & sd)
{
	return const_cast<SCENEDRAW *> (&sd);
}*/

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
