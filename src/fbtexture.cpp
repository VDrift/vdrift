#include "fbtexture.h"
#include <cassert>
#include "opengl_utility.h"
#include <SDL/SDL.h>
#include <sstream>
#include <string>

void FBTEXTURE::Init(GLSTATEMANAGER & glstate, int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, bool usemipmap, std::ostream & error_output, int newmultisample, bool newdepthcomparisonenabled)
{
	assert(!(newalpha && newdepth)); //not allowed; depth maps don't have alpha
	
	assert(!attached);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX init start", error_output);
	
	if (inited)
	{
		DeInit();
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX deinit", error_output);
	}
	
	depth = newdepth;
	depthcomparisonenabled = newdepthcomparisonenabled;
	
	alpha = newalpha;
	
	inited = true;
	
	sizew = sizex;
	sizeh = sizey;
	
	mipmap = usemipmap;
	
	texture_target = target;
	
	multisample = newmultisample;
	if (!(GL_EXT_framebuffer_multisample && GL_EXT_framebuffer_blit))
		multisample = 0;
	
	//set texture info
	if (texture_target == GL_TEXTURE_RECTANGLE)
	{
		assert(GLEW_ARB_texture_rectangle);
	}
	texture_format1 = GL_RGB;
	int texture_format2(GL_RGB);
	int texture_format3(GL_UNSIGNED_BYTE);
	if (depth)
	{
		texture_format1 = GL_DEPTH_COMPONENT16;
		texture_format2 = GL_DEPTH_COMPONENT;
		texture_format3 = GL_UNSIGNED_INT;
	}
	else if (alpha)
	{
		texture_format1 = GL_RGBA;
		texture_format2 = GL_RGBA;
	}
	
	//initialize the texture
	glGenTextures(1, &fbtexture);
	glBindTexture(texture_target, fbtexture);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX texture generation and initial bind", error_output);
	
	if (texture_target == CUBEMAP)
	{
		// generate storage for each of the six sides
		for (int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, texture_format1, sizex, sizey, 0, texture_format2, texture_format3, NULL);
		}
	}
	else
	{
		glTexImage2D(texture_target, 0, texture_format1, sizex, sizey, 0, texture_format2, texture_format3, NULL);
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX texture storage initialization", error_output);
	
	//glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_R, GL_CLAMP);
	
	if (filternearest)
	{
		if (mipmap)
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		if (mipmap)
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	
	if (depth)
	{
		glTexParameteri(texture_target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexParameteri(texture_target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		if (depthcomparisonenabled)
			glTexParameteri(texture_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		else
			glTexParameteri(texture_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX texture setup", error_output);
	
	if (mipmap)
	{
		glGenerateMipmap(texture_target);
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX initial mipmap generation", error_output);
	
	glBindTexture(texture_target, 0); // don't leave the texture bound
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBTEX texture unbinding", error_output);
}

void FBTEXTURE::DeInit()
{
	if (fbtexture > 0)
		glDeleteTextures(1, &fbtexture);
	
	inited = false;
}

void FBTEXTURE::Activate() const
{
	assert(inited);
	
	glBindTexture(texture_target, fbtexture);
}

/*void FBTEXTURE::Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output)
{
	if (depth)
	{
		error_output << "FBTEXTURE::Screenshot not supported for depth FBOs" << std::endl;
		return;
	}
	
	SDL_Surface *temp = NULL;
	unsigned char *pixels;
	int i;

	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, sizew, sizeh, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
		0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
		);

	assert(temp);

	pixels = (unsigned char *) malloc(3 * sizew * sizeh);
	assert(pixels);

	if (single_sample_FBO_for_multisampling)
		single_sample_FBO_for_multisampling->Begin(glstate, error_output);
	else
		Begin(glstate, error_output);
	glReadPixels(0, 0, sizew, sizeh, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	if (single_sample_FBO_for_multisampling)
		single_sample_FBO_for_multisampling->End(error_output);
	else
		End(error_output);

	for (i=0; i<sizeh; i++)
		memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3*sizew * (sizeh-i-1), sizew*3);
	free(pixels);

	SDL_SaveBMP(temp, filename.c_str());
	SDL_FreeSurface(temp);
}*/

void FBTEXTURE::Deactivate() const
{
    glDisable(texture_target);
    glBindTexture(texture_target,0);
}
