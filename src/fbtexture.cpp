#include "fbtexture.h"
#include <cassert>
#include "opengl_utility.h"

void FBTEXTURE_GL::Init(int sizex, int sizey, bool rect, bool newdepth, bool filternearest, bool newalpha, std::ostream & error_output)
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
	
	rectangle = rect;
	
	//initialize framebuffer object (FBO)
	assert(GLEW_EXT_framebuffer_object);
	glGenFramebuffersEXT(1, &framebuffer_object);
	
	//set texture info
	int texture_target(GL_TEXTURE_2D);
	if (rectangle)
	{
		assert(GLEW_ARB_texture_rectangle);
		texture_target = GL_TEXTURE_RECTANGLE_ARB;
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
	glGenTextures(1, &fbtexture);
	glBindTexture(texture_target, fbtexture);
	glTexImage2D(texture_target, 0, texture_format1, sizex, sizey, 0, texture_format2, texture_format3, NULL);
	//glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
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
	
	//bind the framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_object);
	
	//initialize renderbuffer object that's used for our depth buffer when rendering to a texture
	if (!depth)
	{
		glGenRenderbuffersEXT(1, &renderbuffer_depth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_depth);
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
	int texture_attachment(GL_COLOR_ATTACHMENT0_EXT);
	if (depth)
		texture_attachment = GL_DEPTH_ATTACHMENT_EXT;
	
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, texture_attachment, texture_target, fbtexture, 0);
	
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	if (status == GL_FRAMEBUFFER_UNSUPPORTED_EXT)
	{
		error_output << "Unsupported framebuffer format in FBTEXTURE_GL::Init(" << sizex << ", " << sizey << ", " << rect << ", " << depth << ", " << filternearest << ", " << alpha << ")" << std::endl;
	}
	assert(status != GL_FRAMEBUFFER_UNSUPPORTED_EXT);
	assert(status == GL_FRAMEBUFFER_COMPLETE_EXT);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO creation", error_output);
}

void FBTEXTURE_GL::DeInit()
{
	if (inited)
	{
		glDeleteTextures(1, &fbtexture);
		glDeleteFramebuffersEXT(1, &framebuffer_object);
		if (!depth)
			glDeleteRenderbuffersEXT(1, &renderbuffer_depth);
	}
	
	loaded = false;
	inited = false;
}

void FBTEXTURE_GL::Begin(std::ostream & error_output, float viewscale)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_object);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,(int)(sizew*viewscale),(int)(sizeh*viewscale));
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO begin", error_output);
}

void FBTEXTURE_GL::End(std::ostream & error_output)
{
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO end", error_output);
}

void FBTEXTURE_GL::Activate() const
{
	int texture_target(GL_TEXTURE_2D);
	if (rectangle)
		texture_target = GL_TEXTURE_RECTANGLE_ARB;

	glBindTexture(texture_target, fbtexture);
	//glEnable(texture_target);
}
