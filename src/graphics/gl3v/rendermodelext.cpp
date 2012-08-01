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

#include "rendermodelext.h"

RenderModelExternal::RenderModelExternal() : vao(0), elementCount(0), enabled(false)
{
	// Constructor.
}

RenderModelExternal::RenderModelExternal(const RenderModelEntry & m) : vao(m.vao), elementCount(m.elementCount)
{
	if (elementCount > 0)
		enabled = true;
}

RenderModelExternal::~RenderModelExternal()
{
	// Destructor.
}

void RenderModelExternal::draw(GLWrapper & gl) const
{
	gl.drawGeometry(vao, elementCount);
}

bool RenderModelExternal::drawEnabled() const
{
	return enabled;
}

void RenderModelExternal::setVertexArrayObject(GLuint newVao, unsigned int newElementCount)
{
	vao = newVao;
	elementCount = newElementCount;
	if (elementCount > 0)
		enabled = true;
}

void RenderModelExternal::clearTextureCache()
{
	perPassTextureCache.clear();
}

void RenderModelExternal::clearUniformCache()
{
	perPassUniformCache.clear();
}
