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

#ifndef _RENDER_OUTPUT_H
#define _RENDER_OUTPUT_H

#include "fbobject.h"
#include <iosfwd>

class GLSTATEMANAGER;

class RENDER_OUTPUT
{
public:
	RENDER_OUTPUT();

	~RENDER_OUTPUT();

	/// returns the FBO that the user should set up as necessary
	FBOBJECT & RenderToFBO();

	void RenderToFramebuffer();

	bool IsFBO() const;

	void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output);

	void End(GLSTATEMANAGER & glstate, std::ostream & error_output);

private:
	FBOBJECT fbo;
	enum
	{
		RENDER_TO_FBO,
		RENDER_TO_FRAMEBUFFER
	} target;
};

#endif // _RENDER_OUTPUT_H
