#include "fbtexture.h"
#include <cassert>
#include "opengl_utility.h"
#include <SDL/SDL.h>
#include <sstream>
#include <string>

void FBTEXTURE_GL::Init(int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, std::ostream & error_output, int newmultisample)
{
	assert(!(newalpha && newdepth)); //not allowed; depth maps don't have alpha
	
	if (inited)
	{
		DeInit();
	}
	
	depth = newdepth;
	alpha = newalpha;
	
	inited = true;
	
	sizew = sizex;
	sizeh = sizey;
	
	texture_target = target;
	
	multisample = newmultisample;
	if (!(GL_EXT_framebuffer_multisample && GL_EXT_framebuffer_blit))
		multisample = 0;
	
	//initialize framebuffer object (FBO)
	assert(GLEW_EXT_framebuffer_object);
	glGenFramebuffersEXT(1, &framebuffer_object);
	
	//set texture info
	if (texture_target == GL_TEXTURE_RECTANGLE_ARB)
	{
		assert(GLEW_ARB_texture_rectangle);
	}
	int texture_format1(GL_RGB);
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
		//std::cout << "!!!!!!!RGBA" << std::endl;
	}
	
	//initialize the texture
	if (multisample == 0)
	{
		glGenTextures(1, &fbtexture);
		glBindTexture(texture_target, fbtexture);
		
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
		
		//glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		//glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(texture_target, GL_TEXTURE_WRAP_R, GL_CLAMP);
		
		if (filternearest)
		{
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		else
		{
			glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		
		if (depth)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			//if (depthcomparisonenabled)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			//else
			//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		}
		glBindTexture(texture_target, 0); // don't leave the texture bound
	}
	else
	{
		single_sample_FBO_for_multisampling = new FBTEXTURE_GL;
		single_sample_FBO_for_multisampling->Init(sizex, sizey, texture_target, depth, filternearest, alpha, error_output, 0);
	}
	
	//bind the framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_object);
	
	//initialize renderbuffer object that's used for our depth buffer when rendering to a texture
	if (!depth)
	{
		glGenRenderbuffersEXT(1, &renderbuffer_depth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_depth);
		
		if (multisample > 0)
		{
			// need a separate multisample color buffer; can't use our nice single sample texture, unfortunately
			glGenRenderbuffersEXT(1, &renderbuffer_multisample);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_multisample);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, texture_format1, sizew, sizeh);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, renderbuffer_multisample);
			
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_depth);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, GL_DEPTH_COMPONENT, sizew, sizeh);
		}
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, sizew, sizeh);
		
		//attach the render buffer to the FBO
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer_depth);
	}
	else
	{
		glDrawBuffer(GL_NONE); // no color buffer dest
		glReadBuffer(GL_NONE); // no color buffer src
	}
	
	//attach the texture to the FBO
	texture_attachment = GL_COLOR_ATTACHMENT0_EXT;
	if (depth)
		texture_attachment = GL_DEPTH_ATTACHMENT_EXT;
	if (multisample == 0)
	{
		if (texture_target == CUBEMAP)
		{
			// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, texture_attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, fbtexture, 0);
		}
		else
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, texture_attachment, texture_target, fbtexture, 0);
		}
	}
	
	bool status_ok = CheckStatus(error_output);
	assert(status_ok);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO creation", error_output);
}

bool FBTEXTURE_GL::CheckStatus(std::ostream & error_output)
{
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	if (status == GL_FRAMEBUFFER_UNSUPPORTED_EXT)
	{
		error_output << "Unsupported framebuffer format in FBTEXTURE_GL " << sizew << ", " << sizeh << ", " << texture_target << ", " << depth << ", " << alpha << std::endl;
		return false;
	}

	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		error_output << "Framebuffer is not complete: " << status << std::endl;
		return false;
	}
	
	return true;
}

void FBTEXTURE_GL::SetCubeSide(CUBE_SIDE side)
{
	assert(texture_target == CUBEMAP);
	cur_side = side;
}

void FBTEXTURE_GL::DeInit()
{
	if (inited)
	{
		if (multisample == 0)
			glDeleteTextures(1, &fbtexture);
		glDeleteFramebuffersEXT(1, &framebuffer_object);
		if (!depth)
			glDeleteRenderbuffersEXT(1, &renderbuffer_depth);
		if (multisample > 0)
			glDeleteRenderbuffersEXT(1, &renderbuffer_multisample);
		if (single_sample_FBO_for_multisampling)
			delete single_sample_FBO_for_multisampling;
	}
	
	loaded = false;
	inited = false;
}

void FBTEXTURE_GL::Begin(std::ostream & error_output, float viewscale)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("before FBO begin", error_output);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_object);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO bind to framebuffer", error_output);
	
	if (texture_target == CUBEMAP)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, texture_attachment, cur_side, fbtexture, 0);
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO cubemap side attachment", error_output);
	}
	
	bool status_ok = CheckStatus(error_output);
	assert(status_ok);
	
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,(int)(sizew*viewscale),(int)(sizeh*viewscale));
	
	OPENGL_UTILITY::CheckForOpenGLErrors("during FBO begin", error_output);
}

void FBTEXTURE_GL::End(std::ostream & error_output)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("start of FBO end", error_output);
	
	if (single_sample_FBO_for_multisampling)
	{
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, framebuffer_object);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, single_sample_FBO_for_multisampling->framebuffer_object);
		
		assert(glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
		assert(glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample binding", error_output);
		
		glBlitFramebufferEXT(0, 0, sizew, sizeh, 0, 0, sizew, sizeh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample blit", error_output);
		
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, renderbuffer_multisample);
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample block", error_output);
	
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("end of FBO end", error_output);
}

void FBTEXTURE_GL::Activate() const
{
	if (single_sample_FBO_for_multisampling)
	{
		glBindTexture(texture_target, single_sample_FBO_for_multisampling->fbtexture);
	}
	else
		glBindTexture(texture_target, fbtexture);
}

void FBTEXTURE_GL::Screenshot(const std::string & filename, std::ostream & error_output)
{
	if (depth)
	{
		error_output << "FBTEXTURE_GL::Screenshot not supported for depth FBOs" << std::endl;
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
		single_sample_FBO_for_multisampling->Begin(error_output);
	else
		Begin(error_output);
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
}

void FBTEXTURE_GL::Deactivate() const
{
    glDisable(texture_target);
    glBindTexture(texture_target,0);
}
