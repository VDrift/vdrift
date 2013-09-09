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

#include "fbobject.h"

#include "glutil.h"

#include <SDL/SDL.h>

#include <cassert>
#include <sstream>
#include <string>

// allow to run with fbo ext on older gpus (experimental compile time option)
#ifdef FBOEXT

#undef glGenFramebuffers
#undef glBindFramebuffer
#undef glGenRenderbuffers
#undef glBindRenderbuffer
#undef glRenderbufferStorage
#undef glFramebufferRenderbuffer
#undef glFramebufferTexture2D
#undef glCheckFramebufferStatus
#undef glDeleteFramebuffers
#undef glDeleteRenderbuffers

#define glGenFramebuffers GLEW_GET_FUN(__glewGenFramebuffersEXT)
#define glBindFramebuffer GLEW_GET_FUN(__glewBindFramebufferEXT)
#define glGenRenderbuffers GLEW_GET_FUN(__glewGenRenderbuffersEXT)
#define glBindRenderbuffer GLEW_GET_FUN(__glewBindRenderbufferEXT)
#define glRenderbufferStorage GLEW_GET_FUN(__glewRenderbufferStorageEXT)
#define glFramebufferRenderbuffer GLEW_GET_FUN(__glewFramebufferRenderbufferEXT)
#define glFramebufferTexture2D GLEW_GET_FUN(__glewFramebufferTexture2DEXT)
#define glCheckFramebufferStatus GLEW_GET_FUN(__glewCheckFramebufferStatusEXT)
#define glDeleteFramebuffers GLEW_GET_FUN(__glewDeleteFramebuffersEXT)
#define glDeleteRenderbuffers GLEW_GET_FUN(__glewDeleteRenderbuffersEXT)

#endif // FBOEXT

#define _CASE_(x) case FrameBufferTexture::x:\
return #x;

static std::string TargetToString(FrameBufferTexture::TARGET value)
{
	switch (value)
	{
		_CASE_(NORMAL);
		_CASE_(RECTANGLE);
		_CASE_(CUBEMAP);
	}
	return "UNKNOWN";
}

static std::string FormatToString(FrameBufferTexture::FORMAT value)
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

FrameBufferObject::FrameBufferObject() :
	framebuffer_object(0),
	renderbuffer_depth(0),
	inited(false),
	width(0),
	height(0)
{
	// ctor
}

FrameBufferObject::~FrameBufferObject()
{
	DeInit();
}

void FrameBufferObject::Init(GraphicsState & glstate, std::vector <FrameBufferTexture*> newtextures, std::ostream & error_output, bool force_multisample_off)
{
	CheckForOpenGLErrors("FBO init start", error_output);

	const bool verbose = false;

	if (inited)
	{
		if (verbose) error_output << "INFO: deinitializing existing FBO" << std::endl;

		DeInit();

		CheckForOpenGLErrors("FBO deinit", error_output);
	}

	textures = newtextures;

	inited = true;

	assert(!textures.empty());

	// run some sanity checks and find out which textures are for the color attachment and
	// which are for the depth attachment
	std::vector <FrameBufferTexture*> color_textures;
	std::vector <FrameBufferTexture*> depth_textures;

	for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		if ((*i)->format == FrameBufferTexture::DEPTH24)
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

	for (std::vector <FrameBufferTexture*>::iterator i = color_textures.begin(); i != color_textures.end(); i++)
	{
		if ((*i)->target == FrameBufferTexture::CUBEMAP)
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
		for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
		{
			if (multisample == -1)
				multisample = (*i)->multisample;

			//all must have the same multisample
			assert(multisample == (*i)->multisample);
		}
	}

	if (verbose) error_output << "INFO: multisample " << multisample << " found, " << force_multisample_off << std::endl;

	if (force_multisample_off || !GLEW_EXT_framebuffer_multisample)
		multisample = 0;

	//either we have no multisample
	//or multisample and no depth texture
	assert((multisample == 0) || ((multisample > 0) && depth_textures.empty()));

	//ensure consistent sizes
	width = -1;
	height = -1;
	for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
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
	glGenFramebuffers(1, &framebuffer_object);

	if (verbose) error_output << "INFO: generated FBO " << framebuffer_object << std::endl;

	CheckForOpenGLErrors("FBO generation", error_output);

	//bind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	CheckForOpenGLErrors("FBO binding", error_output);

	//initialize renderbuffer object that's used for our depth buffer if we're not using
	//a depth texture
	if (depth_textures.empty())
	{
		glGenRenderbuffers(1, &renderbuffer_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_depth);

		if (verbose) error_output << "INFO: generating depth renderbuffer" << std::endl;

		CheckForOpenGLErrors("FBO renderbuffer generation", error_output);

		if (multisample > 0)
		{
			// need a separate multisample depth buffer
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, GL_DEPTH_COMPONENT24, width, height);

			if (verbose) error_output << "INFO: using multisampling for depth renderbuffer" << std::endl;
		}
		else
		{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		}

		CheckForOpenGLErrors("FBO renderbuffer initialization", error_output);

		//attach the render buffer to the FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_depth);

		if (verbose) error_output << "INFO: depth renderbuffer attached to FBO" << std::endl;

		CheckForOpenGLErrors("FBO renderbuffer attachment", error_output);
	}

	//add separate multisample color buffers for each color texture
	if (multisample > 0)
	{
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			// need a separate multisample color buffer
			glGenRenderbuffers(1, &(*i)->renderbuffer_multisample);
			glBindRenderbuffer(GL_RENDERBUFFER, (*i)->renderbuffer_multisample);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, (*i)->format, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+count, GL_RENDERBUFFER, (*i)->renderbuffer_multisample);

			if (verbose) error_output << "INFO: generating separate multisample color buffer " << count << std::endl;

			CheckForOpenGLErrors("FBO multisample color renderbuffer", error_output);
		}
	}

	// attach any color textures to the FBO
	if (multisample == 0)
	{
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			int attachment = GL_COLOR_ATTACHMENT0+count;
			if ((*i)->target == FrameBufferTexture::CUBEMAP)
			{
				// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (*i)->fbtexture, 0);

				if (verbose) error_output << "INFO: attaching arbitrary cubemap face to color attachment " << count << std::endl;
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, (*i)->target, (*i)->fbtexture, 0);

				if (verbose) error_output << "INFO: attaching texture to color attachment " << count << std::endl;
			}
			(*i)->attachment = attachment;
		}
	}

	// attach the depth texture to the FBO, if there is one
	if (multisample == 0)
	{
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = depth_textures.begin(); i != depth_textures.end(); i++,count++)
		{
			if ((*i)->target == FrameBufferTexture::CUBEMAP)
			{
				// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (*i)->fbtexture, 0);

				if (verbose) error_output << "INFO: attaching cubemap depth texture" << std::endl;
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (*i)->target, (*i)->fbtexture, 0);

				if (verbose) error_output << "INFO: attaching depth texture" << std::endl;
			}
		}
	}

	CheckForOpenGLErrors("FBO attachment", error_output);

	GLenum buffers[4] = {GL_NONE, GL_NONE, GL_NONE, GL_NONE};
	{
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = color_textures.begin(); i != color_textures.end(); i++,count++)
		{
			buffers[count] = GL_COLOR_ATTACHMENT0+count;
		}
	}
	glDrawBuffers(4, buffers);
	glReadBuffer(buffers[0]);

	if (verbose) error_output << "INFO: set draw buffers: " << buffers[0] << ", " << buffers[1] << ", " << buffers[2] << ", " << buffers[3] << std::endl;
	if (verbose) error_output << "INFO: set read buffer: " << buffers[0] << std::endl;

	CheckForOpenGLErrors("FBO buffer mask set", error_output);

	bool status_ok = CheckStatus(error_output);
	if (!status_ok)
	{
		error_output << "Error initializing FBO:" << std::endl;
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
		{
			error_output << "\t" << count << ". " << TargetToString((*i)->target) << ": " << FormatToString((*i)->format) << std::endl;
			count++;
		}
	}
	assert(status_ok);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CheckForOpenGLErrors("FBO unbinding", error_output);

	// if multisampling is on, create another framebuffer object for the single sample version of these textures
	if (multisample > 0)
	{
		if (verbose) error_output << "INFO: creating secondary single sample framebuffer object" << std::endl;

		assert(multisample_dest_singlesample_framebuffer_object.empty());
		multisample_dest_singlesample_framebuffer_object.push_back(FrameBufferObject());
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

bool FrameBufferObject::CheckStatus(std::ostream & error_output)
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		error_output << "Framebuffer is not complete: " << GetStatusString(status) << std::endl;
		return false;
	}

	return true;
}

void FrameBufferObject::SetCubeSide(FrameBufferTexture::CUBE_SIDE side)
{
	assert(textures.size() == 1);
	assert(textures.back()->target == FrameBufferTexture::CUBEMAP);
	textures.back()->cur_side = side;
}

bool FrameBufferObject::IsCubemap() const
{
	return (textures.size() == 1 && textures.back()->target == FrameBufferTexture::CUBEMAP);
}

void FrameBufferObject::DeInit()
{
	if (framebuffer_object > 0)
		glDeleteFramebuffers(1, &framebuffer_object);

	if (renderbuffer_depth > 0)
		glDeleteRenderbuffers(1, &renderbuffer_depth);

	for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		(*i)->attached = false;
		if ((*i)->renderbuffer_multisample > 0)
			glDeleteRenderbuffers(1, &((*i)->renderbuffer_multisample));
	}

	multisample_dest_singlesample_framebuffer_object.clear();

	textures.clear();

	inited = false;
}

void FrameBufferObject::Begin(GraphicsState & glstate, std::ostream & error_output, float viewscale)
{
	CheckForOpenGLErrors("before FBO begin", error_output);

	assert(inited);
	assert(framebuffer_object > 0);
	assert(!textures.empty());

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	CheckForOpenGLErrors("FBO bind to framebuffer", error_output);

	if (textures.back()->target == FrameBufferTexture::CUBEMAP)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, textures.back()->attachment, textures.back()->cur_side, textures.back()->fbtexture, 0);
		CheckForOpenGLErrors("FBO cubemap side attachment", error_output);
	}

	bool status_ok = CheckStatus(error_output);
	assert(status_ok);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,(int)(textures.back()->sizew*viewscale),(int)(textures.back()->sizeh*viewscale));

	CheckForOpenGLErrors("during FBO begin", error_output);
}

void FrameBufferObject::End(GraphicsState & glstate, std::ostream & error_output)
{
	CheckForOpenGLErrors("start of FBO end", error_output);

	assert(!textures.empty());

	//if we're multisampling, copy the result into the single sampled framebuffer, which is attached
	//to the single sample fbtextures
	if (textures.back()->renderbuffer_multisample > 0)
	{
		assert(multisample_dest_singlesample_framebuffer_object.size() == 1);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_object);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, multisample_dest_singlesample_framebuffer_object.back().framebuffer_object);

		assert(glCheckFramebufferStatusEXT(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		assert(glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		CheckForOpenGLErrors("FBO end multisample binding", error_output);

		glBlitFramebufferEXT(0, 0, textures.back()->sizew, textures.back()->sizeh, 0, 0, textures.back()->sizew, textures.back()->sizeh, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		CheckForOpenGLErrors("FBO end multisample blit", error_output);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CheckForOpenGLErrors("FBO multisample blit", error_output);

	glPopAttrib(); //viewport attrib

	// optionally rebuild mipmaps
	for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		if ((*i)->mipmap)
		{
			glBindTexture((*i)->target, (*i)->fbtexture);
			glGenerateMipmap((*i)->target);
			glBindTexture((*i)->target, 0);
		}
	}

	CheckForOpenGLErrors("end of FBO end", error_output);
}
