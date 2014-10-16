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

class GraphicsState;

class RenderOutput
{
public:
	RenderOutput();

	~RenderOutput();

	/// returns the FBO that the user should set up as necessary
	FrameBufferObject & RenderToFBO();

	void RenderToFramebuffer(int width, int height);

	bool IsFBO() const;

	void Begin(GraphicsState & glstate, std::ostream & error_output);

	void End(GraphicsState & glstate, std::ostream & error_output);

private:
	FrameBufferObject fbo;
	bool use_framebuffer;
	int fb_width;
	int fb_height;
};

#endif // _RENDER_OUTPUT_H
