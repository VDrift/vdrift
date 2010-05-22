#include "fbtexture.h"
#include <cassert>
#include "opengl_utility.h"
#include <SDL/SDL.h>
#include <sstream>
#include <string>

void FBTEXTURE::Init(int sizex, int sizey, TARGET target, bool newdepth, bool filternearest, bool newalpha, bool usemipmap, std::ostream & error_output, int newmultisample)
{
	assert(!(newalpha && newdepth)); //not allowed; depth maps don't have alpha
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO init start", error_output);
	
	if (inited)
	{
		DeInit();
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO deinit", error_output);
	}
	
	depth = newdepth;
	alpha = newalpha;
	
	inited = true;
	
	sizew = sizex;
	sizeh = sizey;
	
	mipmap = usemipmap;
	
	texture_target = target;
	
	multisample = newmultisample;
	if (!(GL_EXT_framebuffer_multisample && GL_EXT_framebuffer_blit))
		multisample = 0;
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO generation", error_output);
	
	//initialize framebuffer object (FBO)
	assert(GLEW_ARB_framebuffer_object);
	glGenFramebuffersEXT(1, &framebuffer_object);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO generation", error_output);
	
	//set texture info
	if (texture_target == GL_TEXTURE_RECTANGLE)
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
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO texture generation", error_output);
		
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
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO texture initialization", error_output);
		
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
			//if (depthcomparisonenabled)
				glTexParameteri(texture_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			//else
			//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		}
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO texture setup", error_output);
		
		if (mipmap)
		{
			glGenerateMipmapEXT(texture_target);
		}
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO mipmap generation", error_output);
		
		glBindTexture(texture_target, 0); // don't leave the texture bound
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO texture unbinding", error_output);
	}
	else
	{
		single_sample_FBO_for_multisampling = new FBTEXTURE;
		single_sample_FBO_for_multisampling->Init(sizex, sizey, texture_target, depth, filternearest, alpha, mipmap, error_output, 0);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO multisample creation", error_output);
	}
	
	//bind the framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER, framebuffer_object);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO binding", error_output);
	
	//initialize renderbuffer object that's used for our depth buffer when rendering to a texture
	if (!depth)
	{
		glGenRenderbuffersEXT(1, &renderbuffer_depth);
		glBindRenderbufferEXT(GL_RENDERBUFFER, renderbuffer_depth);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer generation", error_output);
		
		if (multisample > 0)
		{
			// need a separate multisample color buffer; can't use our nice single sample texture, unfortunately
			glGenRenderbuffersEXT(1, &renderbuffer_multisample);
			glBindRenderbufferEXT(GL_RENDERBUFFER, renderbuffer_multisample);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, multisample, texture_format1, sizew, sizeh);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer_multisample);
			
			glBindRenderbufferEXT(GL_RENDERBUFFER, renderbuffer_depth);
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, multisample, GL_DEPTH_COMPONENT, sizew, sizeh);
		}
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, sizew, sizeh);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer initialization", error_output);
		
		//attach the render buffer to the FBO
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_depth);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer attachment", error_output);
	}
	else
	{
		glDrawBuffer(GL_NONE); // no color buffer dest
		glReadBuffer(GL_NONE); // no color buffer src
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO color buffer mask set", error_output);
	}
	
	//attach the texture to the FBO
	texture_attachment = GL_COLOR_ATTACHMENT0;
	if (depth)
		texture_attachment = GL_DEPTH_ATTACHMENT;
	if (multisample == 0)
	{
		if (texture_target == CUBEMAP)
		{
			// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER, texture_attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, fbtexture, 0);
		}
		else
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER, texture_attachment, texture_target, fbtexture, 0);
		}
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO attachment", error_output);
	
	bool status_ok = CheckStatus(error_output);
	assert(status_ok);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO unbinding", error_output);
}

bool FBTEXTURE::CheckStatus(std::ostream & error_output)
{
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
	
	if (status == GL_FRAMEBUFFER_UNSUPPORTED)
	{
		error_output << "Unsupported framebuffer format in FBTEXTURE " << sizew << ", " << sizeh << ", " << texture_target << ", " << depth << ", " << alpha << std::endl;
		return false;
	}

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		error_output << "Framebuffer is not complete: " << status << std::endl;
		return false;
	}
	
	return true;
}

void FBTEXTURE::SetCubeSide(CUBE_SIDE side)
{
	assert(texture_target == CUBEMAP);
	cur_side = side;
}

void FBTEXTURE::DeInit()
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

void FBTEXTURE::Begin(GLSTATEMANAGER & glstate, std::ostream & error_output, float viewscale)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("before FBO begin", error_output);
	
	glstate.BindFramebuffer(framebuffer_object);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO bind to framebuffer", error_output);
	
	if (texture_target == CUBEMAP)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, texture_attachment, cur_side, fbtexture, 0);
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO cubemap side attachment", error_output);
	}
	
	bool status_ok = CheckStatus(error_output);
	assert(status_ok);
	
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,(int)(sizew*viewscale),(int)(sizeh*viewscale));
	
	OPENGL_UTILITY::CheckForOpenGLErrors("during FBO begin", error_output);
}

void FBTEXTURE::End(std::ostream & error_output)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("start of FBO end", error_output);
	
	if (single_sample_FBO_for_multisampling)
	{
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER, framebuffer_object);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, single_sample_FBO_for_multisampling->framebuffer_object);
		
		assert(glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		assert(glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample binding", error_output);
		
		glBlitFramebufferEXT(0, 0, sizew, sizeh, 0, 0, sizew, sizeh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample blit", error_output);
		
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, renderbuffer_multisample);
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample block", error_output);
	
	glPopAttrib();
	
	// optionally rebuild mipmaps
	if (mipmap)
	{
		Activate();
		glGenerateMipmap(texture_target);
		Deactivate();
	}
	
	OPENGL_UTILITY::CheckForOpenGLErrors("end of FBO end", error_output);
}

void FBTEXTURE::Activate() const
{
	if (single_sample_FBO_for_multisampling)
	{
		glBindTexture(texture_target, single_sample_FBO_for_multisampling->fbtexture);
	}
	else
		glBindTexture(texture_target, fbtexture);
}

void FBTEXTURE::Screenshot(GLSTATEMANAGER & glstate, const std::string & filename, std::ostream & error_output)
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
}

void FBTEXTURE::Deactivate() const
{
    glDisable(texture_target);
    glBindTexture(texture_target,0);
}
