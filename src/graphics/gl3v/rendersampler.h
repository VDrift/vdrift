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

#ifndef _RENDERSAMPLER
#define _RENDERSAMPLER

#include "glwrapper.h"
#include "renderstate.h"

class RenderSampler
{
public:
	RenderSampler(GLuint newTu, GLuint newHandle);
	void apply(GLWrapper & gl) const;
	void addState(const RenderState & newState);
	GLuint getHandle() const;
	const std::vector <RenderState> & getStateVector() const; ///< Used for debug printing.

private:
	std::vector <RenderState> state;
	GLuint tu;
	GLuint handle;
};

#endif
