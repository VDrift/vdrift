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

RENDER_OUTPUT::RENDER_OUTPUT() :
	target(RENDER_TO_FRAMEBUFFER)
{
	// ctor
}

RENDER_OUTPUT::~RENDER_OUTPUT()
{
	// ctor
}

FBOBJECT & RENDER_OUTPUT::RenderToFBO()
{
	target = RENDER_TO_FBO;
	return fbo;
}

void RENDER_OUTPUT::RenderToFramebuffer()
{
	target = RENDER_TO_FRAMEBUFFER;
}

bool RENDER_OUTPUT::IsFBO() const
{
	return target == RENDER_TO_FBO;
}

void RENDER_OUTPUT::Begin(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.Begin(glstate, error_output);
	else if (target == RENDER_TO_FRAMEBUFFER)
		glstate.BindFramebuffer(0);
}

void RENDER_OUTPUT::End(GLSTATEMANAGER & glstate, std::ostream & error_output)
{
	if (target == RENDER_TO_FBO)
		fbo.End(glstate, error_output);
}
