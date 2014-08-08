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
#include "graphicsstate.h"
#include "glutil.h"

#include <cassert>
#include <sstream>
#include <string>

#define _CASE_(x) case FrameBufferTexture::x:\
return #x;

static std::string TargetToString(FrameBufferTexture::Target value)
{
	switch (value)
	{
		_CASE_(NORMAL);
		_CASE_(RECTANGLE);
		_CASE_(CUBEMAP);
	}
	return "UNKNOWN";
}

static std::string FormatToString(FrameBufferTexture::Format value)
{
	switch (value)
	{
		_CASE_(R8);
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
	inited(false),
	width(0),
	height(0),
	depth_renderbuffer(0),
	singlesample_framebuffer_object(0)
{
	// ctor
}

FrameBufferObject::~FrameBufferObject()
{
	DeInit();
}

void FrameBufferObject::Init(
	GraphicsState & glstate,
	const std::vector <FrameBufferTexture*> & newtextures,
	std::ostream & error_output,
	bool force_multisample_off)
{
	CheckForOpenGLErrors("FBO init start", error_output);

	const bool verbose = false;

	if (inited)
	{
		if (verbose) error_output << "INFO: deinitializing existing FBO" << std::endl;

		DeInit();

		CheckForOpenGLErrors("FBO deinit", error_output);
	}
	inited = true;

	// need at least some textures
	assert(!newtextures.empty());
	textures = newtextures;

	width = -1;
	height = -1;
	std::vector <FrameBufferTexture*> color_textures;
	FrameBufferTexture * depth_texture = 0;
	for (std::vector <FrameBufferTexture*>::const_iterator i = textures.begin(); i != textures.end(); i++)
	{
		// ensure consistent sizes
		if (width == -1)
			width = (*i)->GetW();
		if (height == -1)
			height = (*i)->GetH();
		assert(width == int((*i)->GetW()));
		assert(height == int((*i)->GetH()));

		// separate textures by type
		if ((*i)->GetFormat() == FrameBufferTexture::DEPTH24)
		{
			// can't have more than one depth attachment
			assert(!depth_texture);
			depth_texture = *i;
		}
		else
		{
			color_textures.push_back(*i);
		}
	}
	if (verbose) error_output << "INFO: width " << width << ", height " << height << std::endl;
	if (verbose) error_output << "INFO: color textures: " << color_textures.size() << std::endl;
	if (verbose && depth_texture) error_output << "INFO: depth texture: 1" << std::endl;

	// can't have more than 4 color attachments
	assert(color_textures.size() < 5);

	// check for cubemaps
	for (std::vector <FrameBufferTexture*>::const_iterator i = color_textures.begin(); i != color_textures.end(); i++)
	{
		if ((*i)->GetTarget() == FrameBufferTexture::CUBEMAP)
		{
			if (verbose) error_output << "INFO: found cubemap" << std::endl;

			// can't have MRT with cubemaps
			assert(color_textures.size() == 1);

			// can't have multisample with cubemaps
			assert((*i)->GetMultiSample() == 0);

			// can't have depth texture with cubemaps
			assert(!depth_texture);
		}
	}

	// find what multisample value to use
	int multisample = 0;
	if (!color_textures.empty())
	{
		multisample = -1;
		for (std::vector <FrameBufferTexture*>::const_iterator i = textures.begin(); i != textures.end(); i++)
		{
			if (multisample == -1)
				multisample = (*i)->GetMultiSample();

			// all must have the same multisample
			assert(multisample == (*i)->GetMultiSample());
		}
	}

	if (verbose) error_output << "INFO: multisample " << multisample << " found, " << force_multisample_off << std::endl;

	if (force_multisample_off)
		multisample = 0;

	// either we have no multisample or multisample and no depth texture
	assert((multisample == 0) || ((multisample > 0) && depth_texture));

	// initialize framebuffer object
	glGenFramebuffers(1, &framebuffer_object);

	if (verbose) error_output << "INFO: generated FBO " << framebuffer_object << std::endl;

	CheckForOpenGLErrors("FBO generation", error_output);

	// bind framebuffer
	glstate.BindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	CheckForOpenGLErrors("FBO binding", error_output);

	if (!depth_texture)
	{
		// create depth render buffer if we're not using a depth texture
		glGenRenderbuffers(1, &depth_renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);

		if (verbose) error_output << "INFO: generating depth renderbuffer" << std::endl;

		CheckForOpenGLErrors("FBO renderbuffer generation", error_output);

		if (multisample > 0)
		{
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, GL_DEPTH_COMPONENT, width, height);

			if (verbose) error_output << "INFO: using multisampling for depth renderbuffer" << std::endl;
		}
		else
		{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		}

		CheckForOpenGLErrors("FBO renderbuffer initialization", error_output);

		// attach depth render buffer to the FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);

		if (verbose) error_output << "INFO: depth renderbuffer attached to FBO" << std::endl;

		CheckForOpenGLErrors("FBO renderbuffer attachment", error_output);
	}

	if (multisample > 0)
	{
		// create/attach separate multisample color buffers for each color texture
		multisample_renderbuffers.resize(color_textures.size(), 0);
		for (size_t i = 0; i < color_textures.size(); i++)
		{
			glGenRenderbuffers(1, &multisample_renderbuffers[i]);
			glBindRenderbuffer(GL_RENDERBUFFER, multisample_renderbuffers[i]);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample, color_textures[i]->GetFormat(), width, height);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, multisample_renderbuffers[i]);

			if (verbose) error_output << "INFO: generating separate multisample color buffer " << i << std::endl;

			CheckForOpenGLErrors("FBO multisample color renderbuffer", error_output);
		}
	}
	else
	{
		// attach color textures to frame buffer object
		int count = 0;
		for (std::vector <FrameBufferTexture*>::iterator i = color_textures.begin(); i != color_textures.end(); i++, count++)
		{
			int attachment = GL_COLOR_ATTACHMENT0 + count;
			if ((*i)->GetTarget() == FrameBufferTexture::CUBEMAP)
			{
				// if we're using a cubemap, arbitrarily pick one of the faces to activate so we can check that the FBO is complete
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (*i)->GetId(), 0);

				if (verbose) error_output << "INFO: attaching arbitrary cubemap face to color attachment " << count << std::endl;
			}
			else
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, (*i)->GetTarget(), (*i)->GetId(), 0);

				if (verbose) error_output << "INFO: attaching texture to color attachment " << count << std::endl;
			}
			(*i)->SetAttachment(attachment);

			CheckForOpenGLErrors("FBO attachment", error_output);
		}

		if (depth_texture)
		{
			// attach depth texture to frame buffer object
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture->GetTarget(), depth_texture->GetId(), 0);
			depth_texture->SetAttachment(GL_DEPTH_ATTACHMENT);

			if (verbose) error_output << "INFO: attaching depth texture" << std::endl;
			CheckForOpenGLErrors("FBO attachment", error_output);
		}
	}

	GLenum buffers[4] = {GL_NONE, GL_NONE, GL_NONE, GL_NONE};
	{
		int count = 0;
		for (std::vector <FrameBufferTexture*>::const_iterator i = color_textures.begin(); i != color_textures.end(); i++, count++)
		{
			buffers[count] = GL_COLOR_ATTACHMENT0 + count;
		}

		glDrawBuffers(count, buffers);
		glReadBuffer(buffers[0]);

		CheckForOpenGLErrors("FBO buffer mask set", error_output);
	}

	if (verbose) error_output << "INFO: set draw buffers: " << buffers[0] << ", " << buffers[1] << ", " << buffers[2] << ", " << buffers[3] << std::endl;
	if (verbose) error_output << "INFO: set read buffer: " << buffers[0] << std::endl;

	if (!CheckStatus(error_output))
	{
		error_output << "Error initializing FBO:" << std::endl;
		int count = 0;
		for (std::vector <FrameBufferTexture*>::const_iterator i = textures.begin(); i != textures.end(); i++)
		{
			error_output << "\t" << count << ". " << TargetToString(FrameBufferTexture::Target((*i)->GetTarget()));
			error_output << ": " << FormatToString((*i)->GetFormat()) << std::endl;
			count++;
		}
		assert(0);
	}

	// explicitely unbind framebuffer object
	glstate.BindFramebuffer(GL_FRAMEBUFFER, 0);

	CheckForOpenGLErrors("FBO unbinding", error_output);

	// if multisampling is on, create another framebuffer object for the single sample version of these textures
	if (multisample > 0)
	{
		if (verbose) error_output << "INFO: creating secondary single sample framebuffer object" << std::endl;

		assert(!singlesample_framebuffer_object);
		singlesample_framebuffer_object = new FrameBufferObject();
		singlesample_framebuffer_object->Init(glstate, newtextures, error_output, true);
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

void FrameBufferObject::SetCubeSide(FrameBufferTexture::CubeSide side)
{
	assert(textures.size() == 1);
	textures.back()->SetSide(side);
}

bool FrameBufferObject::IsCubemap() const
{
	return (textures.size() == 1 && textures.back()->GetTarget() == FrameBufferTexture::CUBEMAP);
}

void FrameBufferObject::DeInit()
{
	if (framebuffer_object)
	{
		glDeleteFramebuffers(1, &framebuffer_object);
		framebuffer_object = 0;
	}

	for (std::vector <FrameBufferTexture*>::iterator i = textures.begin(); i != textures.end(); i++)
	{
		(*i)->SetAttachment(0);
	}
	textures.clear();

	for (std::vector<GLuint>::const_iterator i = multisample_renderbuffers.begin(); i != multisample_renderbuffers.end(); i++)
	{
		if (*i > 0)
			glDeleteRenderbuffers(1, &*i);
	}
	multisample_renderbuffers.clear();

	if (depth_renderbuffer)
	{
		glDeleteRenderbuffers(1, &depth_renderbuffer);
		depth_renderbuffer = 0;
	}

	if (singlesample_framebuffer_object)
	{
		delete singlesample_framebuffer_object;
		singlesample_framebuffer_object = 0;
	}

	inited = false;
	width = 0;
	height = 0;
}

void FrameBufferObject::Begin(GraphicsState & glstate, std::ostream & error_output, float viewscale)
{
	CheckForOpenGLErrors("before FBO begin", error_output);

	assert(inited);
	assert(framebuffer_object > 0);
	assert(!textures.empty());

	glstate.BindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	CheckForOpenGLErrors("FBO bind to framebuffer", error_output);

	FrameBufferTexture * tex = textures.back();
	if (tex->GetTarget() == FrameBufferTexture::CUBEMAP)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, tex->GetAttachment(), tex->GetSide(), tex->GetId(), 0);
		CheckForOpenGLErrors("FBO cubemap side attachment", error_output);
	}

	assert(CheckStatus(error_output));

	glstate.SetViewport(int(tex->GetW() * viewscale), int(tex->GetH() * viewscale));

	CheckForOpenGLErrors("during FBO begin", error_output);
}

void FrameBufferObject::End(GraphicsState & glstate, std::ostream & error_output)
{
	CheckForOpenGLErrors("start of FBO end", error_output);

	if (singlesample_framebuffer_object)
	{
		glstate.BindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_object);
		glstate.BindFramebuffer(GL_DRAW_FRAMEBUFFER, singlesample_framebuffer_object->framebuffer_object);

		assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		CheckForOpenGLErrors("FBO end multisample binding", error_output);

		const int w = singlesample_framebuffer_object->GetWidth();
		const int h = singlesample_framebuffer_object->GetHeight();
		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		CheckForOpenGLErrors("FBO end multisample blit", error_output);
	}

	CheckForOpenGLErrors("FBO multisample blit", error_output);

	// optionally rebuild mipmaps
	for (std::vector <FrameBufferTexture*>::const_iterator i = textures.begin(); i != textures.end(); i++)
	{
		if ((*i)->HasMipMap())
		{
			glstate.BindTexture(0, (*i)->GetTarget(), (*i)->GetId());
			glGenerateMipmap((*i)->GetTarget());
			glstate.BindTexture(0, (*i)->GetTarget(), 0);
		}
	}

	CheckForOpenGLErrors("end of FBO end", error_output);
}
