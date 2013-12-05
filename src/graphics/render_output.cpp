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

#include "render_output.h"
#include "graphicsstate.h"

RenderOutput::RenderOutput() :
	use_framebuffer(true),
	fb_width(0),
	fb_height(0)
{
	// ctor
}

RenderOutput::~RenderOutput()
{
	// ctor
}

FrameBufferObject & RenderOutput::RenderToFBO()
{
	use_framebuffer = false;
	return fbo;
}

void RenderOutput::RenderToFramebuffer(int width, int height)
{
	use_framebuffer = true;
	fb_width = width;
	fb_height = height;
}

bool RenderOutput::IsFBO() const
{
	return !use_framebuffer;
}

void RenderOutput::Begin(GraphicsState & glstate, std::ostream & error_output)
{
	if (!use_framebuffer)
	{
		fbo.Begin(glstate, error_output);
	}
	else
	{
		assert(fb_width != 0 && fb_height != 0);
		glstate.BindFramebuffer(GL_FRAMEBUFFER, 0);
		glstate.SetViewport(fb_width, fb_height);
	}
}

void RenderOutput::End(GraphicsState & glstate, std::ostream & error_output)
{
	if (!use_framebuffer)
	{
		fbo.End(glstate, error_output);
	}
}
