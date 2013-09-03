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
#include "glstatemanager.h"

RenderOutput::RenderOutput() :
	target(RENDER_TO_FRAMEBUFFER)
{
	// ctor
}

RenderOutput::~RenderOutput()
{
	// ctor
}

FrameBufferObject & RenderOutput::RenderToFBO()
{
	target = RENDER_TO_FBO;
	return fbo;
}

void RenderOutput::RenderToFramebuffer()
{
	target = RENDER_TO_FRAMEBUFFER;
}

bool RenderOutput::IsFBO() const
{
	return target == RENDER_TO_FBO;
}

void RenderOutput::Begin(GraphicsState & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.Begin(glstate, error_output);
}

void RenderOutput::End(GraphicsState & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.End(glstate, error_output);
}
