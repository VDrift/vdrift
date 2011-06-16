#include "fbobject.h"

#include "opengl_utility.h"

#include <SDL/SDL.h>

#include <cassert>
#include <sstream>
#include <string>

#define _CASE_(x) case FBTEXTURE::x:\
return #x;

std::string TargetToString(FBTEXTURE::TARGET value)
{
	switch (value)
	{
		_CASE_(NORMAL);
		_CASE_(RECTANGLE);
		_CASE_(CUBEMAP);
	}
	return "UNKNOWN";
}

std::string FormatToString(FBTEXTURE::FORMAT value)
{
	switch (value)
	{
		_CASE_(LUM8);
		_CASE_(RGB8);
		_CASE_(RGBA8);
		_CASE_(RGB16);
		_CASE_(RGBA16);
		_CASE_(DEPTH24);
	}
	return "UNKNOWN";
}

#undef _CASE_

void FBOBJECT::Init(GLSTATEMANAGER & glstate, std::vector <FBTEXTURE*> newtextures, std::ostream & error_output, bool force_multisample_off)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("FBO init start", error_output);

	const bool verbose = false;

	if (inited)
	{
		if (verbose) error_output << "INFO: deinitializing existing FBO" << std::endl;

		DeInit();

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO deinit", error_output);
	}

	textures = newtextures;

	inited = true;

	assert(!textures.empty());

	// run some sanity checks and find out which textures are for the color attachment and
	// which are for the depth attachment
	std::vector <FBTEXTURE*> color_textures;
	std::vector <FBTEXTURE*> depth_textures;

	for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		if ((*i)->texture_format == FBTEXTURE::DEPTH24)
			depth_textures.push_back(*i);
		else
			color_textures.push_back(*i);

		//assert(!(*i)->attached || (force_multisample_off && (*i)->renderbuffer_multisample != 0));
		(*i)->attached = true;
	}

	if (verbose) error_output << "INFO: color textures: " << color_textures.size() << ", depth textures: " << depth_textures.size() << std::endl;

	//need at least some textures
	assert(color_textures.size() + depth_textures.size() >= 1);

	//can't have more than one depth attachment
	assert(depth_textures.size() < 2);

	//can't have more than 4 color textures
	assert(color_textures.size() < 5);

	for (std::vector <FBTEXTURE*>::iterator i = color_textures.begin(); i != color_textures.end(); i++)
	{
		if ((*i)->texture_target == FBTEXTURE::CUBEMAP)
		{
			if (verbose) error_output << "INFO: found cubemap" << std::endl;

			//can't have MRT with cubemaps
			assert(color_textures.size() == 1);

			//can't have multisample with cubemaps
			assert((*i)->multisample == 0);

			//can't have depth texture with cubemaps
			assert(depth_textures.empty());
		}
	}

	//find what multisample value to use
	int multisample = 0;
	if (!color_textures.empty())
	{
		multisample = -1;
		for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
		{
			if (multisample == -1)
				multisample = (*i)->multisample;

			//all must have the same multisample
			assert(multisample == (*i)->multisample);
		}
	}

	if (verbose) error_output << "INFO: multisample " << multisample << " found, " << force_multisample_off << std::endl;

	if (force_multisample_off)
		multisample = 0;

	//either we have no multisample
	//or multisample and no depth texture
	assert((multisample == 0) || ((multisample > 0) && depth_textures.empty()));

	//ensure consistent sizes
	width = -1;
	height = -1;
	for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		if (width == -1)
			width = (*i)->sizew;
		if (height == -1)
			height = (*i)->sizeh;
		assert(width == (*i)->sizew);
		assert(height == (*i)->sizeh);
	}

	if (verbose) error_output << "INFO: width " << width << ", height " << height << std::endl;

	//initialize framebuffer object (FBO)
	assert(GLEW_ARB_framebuffer_object);
	glGenFramebuffers(1, &framebuffer_object);

	if (verbose) error_output << "INFO: generated FBO " << framebuffer_object << std::endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO generation", error_output);

	//bind the framebuffer
	glstate.BindFramebuffer(framebuffer_object);

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO binding", error_output);

	//initialize renderbuffer object that's used for our depth buffer if we're not using
	//a depth texture
	if (depth_textures.empty())
	{
		glGenRenderbuffers(1, &renderbuffer_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_depth);

		if (verbose) error_output << "INFO: generating depth renderbuffer" << std::endl;

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer generation", error_output);

		if (multisample > 0)
		{
			// need a separate multisample depth buffer
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, GL_DEPTH_COMPONENT, width, height);

			if (verbose) error_output << "INFO: using multisampling for depth renderbuffer" << std::endl;
		}
		else
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer initialization", error_output);

		//attach the render buffer to the FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_depth);

		if (verbose) error_output << "INFO: depth renderbuffer attached to FBO" << std::endl;

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO renderbuffer attachment", error_output);
	}

	GLenum buffers[4] = {GL_NONE, GL_NONE, GL_NONE, GL_NONE};
	{
		int count = 0;
		for (std::vector <FBTEXTURE*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			buffers[count] = GL_COLOR_ATTACHMENT0+count;
		}
	}

	glDrawBuffers(4, buffers);
	glReadBuffer(buffers[0]);

	if (verbose) error_output << "INFO: set draw buffers: " << buffers[0] << ", " << buffers[1] << ", " << buffers[2] << ", " << buffers[3] << std::endl;
	if (verbose) error_output << "INFO: set read buffer: " << buffers[0] << std::endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO buffer mask set", error_output);

	//add separate multisample color buffers for each color texture
	if (multisample > 0)
	{
		int count = 0;
		for (std::vector <FBTEXTURE*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			// need a separate multisample color buffer
			glGenRenderbuffers(1, &(*i)->renderbuffer_multisample);
			glBindRenderbuffer(GL_RENDERBUFFER, (*i)->renderbuffer_multisample);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, (*i)->texture_format, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+count, GL_RENDERBUFFER, (*i)->renderbuffer_multisample);

			if (verbose) error_output << "INFO: generating separate multisample color buffer " << count << std::endl;

			OPENGL_UTILITY::CheckForOpenGLErrors("FBO multisample color renderbuffer", error_output);
		}
	}

	//attach any color textures to the FBO
	{
		int count = 0;
		for (std::vector <FBTEXTURE*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			int texture_attachment = GL_COLOR_ATTACHMENT0+count;
			if (multisample == 0)
			{
				if ((*i)->texture_target == FBTEXTURE::CUBEMAP)
				{
					// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
					glFramebufferTexture2D(GL_FRAMEBUFFER, texture_attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (*i)->fbtexture, 0);

					if (verbose) error_output << "INFO: attaching arbitrary cubemap face to color attachment " << count << std::endl;
				}
				else
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, texture_attachment, (*i)->texture_target, (*i)->fbtexture, 0);

					if (verbose) error_output << "INFO: attaching texture to color attachment " << count << std::endl;
				}
				(*i)->texture_attachment = texture_attachment;
			}
		}
	}

	//attach the depth texture to the FBO, if there is one
	{
		int count = 0;
		for (std::vector <FBTEXTURE*>::iterator i = depth_textures.begin(); i != depth_textures.end(); i++,count++)
		{
			if (multisample == 0)
			{
				if ((*i)->texture_target == FBTEXTURE::CUBEMAP)
				{
					// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (*i)->fbtexture, 0);

					if (verbose) error_output << "INFO: attaching cubemap depth texture" << std::endl;
				}
				else
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (*i)->texture_target, (*i)->fbtexture, 0);

					if (verbose) error_output << "INFO: attaching depth texture" << std::endl;
				}
			}
		}
	}

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO attachment", error_output);

	bool status_ok = CheckStatus(error_output);
	if (!status_ok)
	{
		error_output << "Error initializing FBO:" << std::endl;
		int count = 0;
		for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
		{
			error_output << "\t" << count << ". " << TargetToString((*i)->texture_target) << ": " << FormatToString((*i)->texture_format) << std::endl;
			count++;
		}
	}
	assert(status_ok);

	glstate.BindFramebuffer(0);

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO unbinding", error_output);

	// if multisampling is on, create another framebuffer object for the single sample version of these textures
	if (multisample > 0)
	{
		if (verbose) error_output << "INFO: creating secondary single sample framebuffer object" << std::endl;

		assert(multisample_dest_singlesample_framebuffer_object.empty());
		multisample_dest_singlesample_framebuffer_object.push_back(FBOBJECT());
		multisample_dest_singlesample_framebuffer_object.back().Init(glstate, newtextures, error_output, true);
	}
}

std::string GetStatusString(GLenum status)
{
	switch (status)
	{
		case GL_FRAMEBUFFER_UNDEFINED:
		return "framebuffer undefined";

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "incomplete attachment";

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "incomplete draw buffer";

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "incomplete read buffer";

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "missing attachments";

		case GL_FRAMEBUFFER_UNSUPPORTED:
		return "unsupported format";

		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "incomplete multisample";

		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "incomplete layer targets";
	}

	return "unknown error";
}

bool FBOBJECT::CheckStatus(std::ostream & error_output)
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		error_output << "Framebuffer is not complete: " << GetStatusString(status) << std::endl;
		return false;
	}

	return true;
}

void FBOBJECT::SetCubeSide(FBTEXTURE::CUBE_SIDE side)
{
	assert(textures.size() == 1);
	assert(textures.back()->texture_target == FBTEXTURE::CUBEMAP);
	textures.back()->cur_side = side;
}

bool FBOBJECT::IsCubemap() const
{
	if (textures.size() == 1 && textures.back()->texture_target == FBTEXTURE::CUBEMAP)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void FBOBJECT::DeInit()
{
	if (framebuffer_object > 0)
		glDeleteFramebuffers(1, &framebuffer_object);

	if (renderbuffer_depth > 0)
		glDeleteRenderbuffers(1, &renderbuffer_depth);

	for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		(*i)->attached = false;
		if ((*i)->renderbuffer_multisample > 0)
			glDeleteRenderbuffers(1, &((*i)->renderbuffer_multisample));
	}

	multisample_dest_singlesample_framebuffer_object.clear();

	textures.clear();

	inited = false;
}

void FBOBJECT::Begin(GLSTATEMANAGER & glstate, std::ostream & error_output, float viewscale)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("before FBO begin", error_output);

	assert(inited);
	assert(framebuffer_object > 0);
	assert(!textures.empty());

	glstate.BindFramebuffer(framebuffer_object);

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO bind to framebuffer", error_output);

	if (textures.back()->texture_target == FBTEXTURE::CUBEMAP)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, textures.back()->texture_attachment, textures.back()->cur_side, textures.back()->fbtexture, 0);
		OPENGL_UTILITY::CheckForOpenGLErrors("FBO cubemap side attachment", error_output);
	}

	bool status_ok = CheckStatus(error_output);
	assert(status_ok);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,(int)(textures.back()->sizew*viewscale),(int)(textures.back()->sizeh*viewscale));

	OPENGL_UTILITY::CheckForOpenGLErrors("during FBO begin", error_output);
}

void FBOBJECT::End(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("start of FBO end", error_output);

	assert(!textures.empty());

	//if we're multisampling, copy the result into the single sampled framebuffer, which is attached
	//to the single sample fbtextures
	if (textures.back()->renderbuffer_multisample > 0)
	{
		assert(multisample_dest_singlesample_framebuffer_object.size() == 1);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_object);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, multisample_dest_singlesample_framebuffer_object.back().framebuffer_object);

		assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample binding", error_output);

		glBlitFramebuffer(0, 0, textures.back()->sizew, textures.back()->sizeh, 0, 0, textures.back()->sizew, textures.back()->sizeh, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		OPENGL_UTILITY::CheckForOpenGLErrors("FBO end multisample blit", error_output);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glstate.BindFramebuffer(0);

	OPENGL_UTILITY::CheckForOpenGLErrors("FBO multisample blit", error_output);

	glPopAttrib(); //viewport attrib

	// optionally rebuild mipmaps
	for (std::vector <FBTEXTURE*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		if ((*i)->mipmap)
		{
			glBindTexture((*i)->texture_target, (*i)->fbtexture);
			glGenerateMipmap((*i)->texture_target);
			glBindTexture((*i)->texture_target, 0);
		}
	}

	OPENGL_UTILITY::CheckForOpenGLErrors("end of FBO end", error_output);
}
