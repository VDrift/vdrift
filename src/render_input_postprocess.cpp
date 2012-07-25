#include "render_input_postprocess.h"
#include "glutil.h"
#include "glstatemanager.h"
#include "matrix4.h"
#include "shader.h"

RENDER_INPUT_POSTPROCESS::RENDER_INPUT_POSTPROCESS() :
	shader(NULL),
	writealpha(true),
	writecolor(true),
	writedepth(false),
	depth_mode(GL_LEQUAL),
	clearcolor(false),
	cleardepth(false),
	blendmode(BLENDMODE::DISABLED),
	contrast(1.0)
{
	//ctor
}

RENDER_INPUT_POSTPROCESS::~RENDER_INPUT_POSTPROCESS()
{
	//dtor
}

void RENDER_INPUT_POSTPROCESS::SetSourceTextures(const std::vector <TEXTURE_INTERFACE*> & textures)
{
	source_textures = textures;
}

void RENDER_INPUT_POSTPROCESS::SetShader(SHADER_GLSL * newshader)
{
	shader = newshader;
}

void RENDER_INPUT_POSTPROCESS::Render(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	assert(shader);

	GLUTIL::CheckForOpenGLErrors("postprocess begin", error_output);

	glstate.SetColorMask(writecolor, writealpha);
	glstate.SetDepthMask(writedepth);

	if (clearcolor && cleardepth)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if (clearcolor)
		glClear(GL_COLOR_BUFFER_BIT);
	else if (cleardepth)
		glClear(GL_DEPTH_BUFFER_BIT);

	shader->Enable();

	GLUTIL::CheckForOpenGLErrors("postprocess shader enable", error_output);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(1,1,1,1);
	glstate.SetColor(1,1,1,1);

	assert(blendmode != BLENDMODE::ALPHATEST);
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
			glstate.SetBlendFunc(GL_ONE, GL_ONE);
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

		default:
		assert(0);
		break;
	}

	if (writedepth || depth_mode != GL_ALWAYS)
		glstate.Enable(GL_DEPTH_TEST);
	else
		glstate.Disable(GL_DEPTH_TEST);
	glDepthFunc( depth_mode );
	glstate.Enable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

	GLUTIL::CheckForOpenGLErrors("postprocess flag set", error_output);

	// put the camera transform into texture3
	glActiveTexture(GL_TEXTURE3);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	float temp_matrix[16];
	(cam_rotation).GetMatrix4(temp_matrix);
	glLoadMatrixf(temp_matrix);
	glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);
	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_MODELVIEW);

	//std::cout << "postprocess: " << std::endl;
	PushShadowMatrices();

	GLUTIL::CheckForOpenGLErrors("shader parameter upload", error_output);

	float maxu = 1.f;
	float maxv = 1.f;

	int num_nonnull = 0;
	for (unsigned int i = 0; i < source_textures.size(); i++)
	{
		//std::cout << i << ": " << source_textures[i] << std::endl;
		glActiveTexture(GL_TEXTURE0+i);
		if (source_textures[i])
		{
			source_textures[i]->Activate();
			num_nonnull++;
			if (source_textures[i]->IsRect())
			{
				maxu = source_textures[i]->GetW();
				maxv = source_textures[i]->GetH();
			}
		}
	}
	if (num_nonnull <= 0)
	{
		error_output << "Out of the " << source_textures.size() << " input textures provided as inputs to this postprocess stage, zero are available. This stage will have no effect." << std::endl;
		return;
	}
	glActiveTexture(GL_TEXTURE0);

	GLUTIL::CheckForOpenGLErrors("postprocess texture set", error_output);

	// build the frustum corners
	float ratio = w/h;
	std::vector <MATHVECTOR <float, 3> > frustum_corners(4);
	frustum_corners[0].Set(-lod_far,-lod_far,-lod_far);	//BL
	frustum_corners[1].Set(lod_far,-lod_far,-lod_far);	//BR
	frustum_corners[2].Set(lod_far,lod_far,-lod_far);	//TR
	frustum_corners[3].Set(-lod_far,lod_far,-lod_far);	//TL
	MATRIX4 <float> invproj;
	invproj.InvPerspective(camfov, ratio, 0.1, lod_far);
	for (int i = 0; i < 4; i++)
	{
		invproj.TransformVectorOut(frustum_corners[i][0], frustum_corners[i][1], frustum_corners[i][2]);
		frustum_corners[i][2] = -lod_far;
	}

	// send shader parameters
	{
		MATHVECTOR <float, 3> lightvec = lightposition;
		cam_rotation.RotateVector(lightvec);
		shader->UploadActiveShaderParameter3f("directlight_eyespace_direction", lightvec);
		shader->UploadActiveShaderParameter1f("contrast", contrast);
		shader->UploadActiveShaderParameter1f("znear", 0.1);
		//std::cout << lightvec << std::endl;
		shader->UploadActiveShaderParameter3f("frustum_corner_bl", frustum_corners[0]);
		shader->UploadActiveShaderParameter3f("frustum_corner_br_delta", frustum_corners[1]-frustum_corners[0]);
		shader->UploadActiveShaderParameter3f("frustum_corner_tl_delta", frustum_corners[3]-frustum_corners[0]);
	}


	glBegin(GL_QUADS);

	// send the UV corners in UV set 0, send the frustum corners in UV set 1

	glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
	glMultiTexCoord3f(GL_TEXTURE1, frustum_corners[0][0], frustum_corners[0][1], frustum_corners[0][2]);
	glVertex3f( 0.0f,  0.0f,  0.0f);

	glMultiTexCoord2f(GL_TEXTURE0, maxu, 0.0f);
	glMultiTexCoord3f(GL_TEXTURE1, frustum_corners[1][0], frustum_corners[1][1], frustum_corners[1][2]);
	glVertex3f( 1.0f,  0.0f,  0.0f);

	glMultiTexCoord2f(GL_TEXTURE0, maxu, maxv);
	glMultiTexCoord3f(GL_TEXTURE1, frustum_corners[2][0], frustum_corners[2][1], frustum_corners[2][2]);
	glVertex3f( 1.0f,  1.0f,  0.0f);

	glMultiTexCoord2f(GL_TEXTURE0, 0.0f, maxv);
	glMultiTexCoord3f(GL_TEXTURE1, frustum_corners[3][0], frustum_corners[3][1], frustum_corners[3][2]);
	glVertex3f( 0.0f,  1.0f,  0.0f);

	glEnd();

	GLUTIL::CheckForOpenGLErrors("postprocess draw", error_output);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	PopShadowMatrices();

	glstate.Enable(GL_DEPTH_TEST);
	glstate.Disable(GL_TEXTURE_2D);

	for (unsigned int i = 0; i < source_textures.size(); i++)
	{
		//std::cout << i << ": " << source_textures[i] << std::endl;
		glActiveTexture(GL_TEXTURE0+i);
		if (source_textures[i])
			source_textures[i]->Deactivate();
	}
	glActiveTexture(GL_TEXTURE0);

	GLUTIL::CheckForOpenGLErrors("postprocess end", error_output);
}

void RENDER_INPUT_POSTPROCESS::SetWriteColor(bool write)
{
	writecolor = write;
}

void RENDER_INPUT_POSTPROCESS::SetWriteAlpha(bool write)
{
	writealpha = write;
}

void RENDER_INPUT_POSTPROCESS::SetWriteDepth(bool write)
{
	writedepth = write;
}

void RENDER_INPUT_POSTPROCESS::SetDepthMode(int mode)
{
	depth_mode = mode;
}

void RENDER_INPUT_POSTPROCESS::SetClear(bool newclearcolor, bool newcleardepth)
{
	clearcolor = newclearcolor;
	cleardepth = newcleardepth;
}

void RENDER_INPUT_POSTPROCESS::SetBlendMode(BLENDMODE::BLENDMODE mode)
{
	blendmode = mode;
}

void RENDER_INPUT_POSTPROCESS::SetContrast(float value)
{
	contrast = value;
}

void RENDER_INPUT_POSTPROCESS::SetCameraInfo(
	const MATHVECTOR <float, 3> & newpos,
	const QUATERNION <float> & newrot,
	float newfov, float newlodfar,
	float neww, float newh)
{
	cam_position = newpos;
	cam_rotation = newrot;
	camfov = newfov;
	lod_far = newlodfar;
	w = neww;
	h = newh;

	const bool restore_matrices = true;
	glMatrixMode( GL_PROJECTION );
	if (restore_matrices)
		glPushMatrix();
	glLoadIdentity();
	/*if (orthomode)
	{
		glOrtho(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
	}
	else*/
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
}

void RENDER_INPUT_POSTPROCESS::SetSunDirection(const MATHVECTOR <float, 3> & newsun)
{
	lightposition = newsun;
}
