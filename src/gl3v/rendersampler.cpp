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

#include "rendersampler.h"

RenderSampler::RenderSampler(GLuint newTu, GLuint newHandle) : tu(newTu), handle(newHandle)
{
	// Constructor.
}

void RenderSampler::apply(GLWrapper & gl) const
{
	for (std::vector <RenderState>::const_iterator s = state.begin(); s != state.end(); s++)
		s->applySampler(gl, handle);

	gl.BindSampler(tu, handle);
}

void RenderSampler::addState(const RenderState & newState)
{
	state.push_back(newState);
}

GLuint RenderSampler::getHandle() const
{
	return handle;
}

const std::vector <RenderState> & RenderSampler::getStateVector() const
{
	return state;
}
