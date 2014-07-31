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

#ifndef _RENDER_MODEL_EXT_DRAWABLE
#define _RENDER_MODEL_EXT_DRAWABLE

#include "gl3v/rendermodelext.h"
#include "vertexbuffer.h"

class RenderModelExtDrawable : public RenderModelExt
{
friend class Drawable;
public:
	virtual void draw(GLWrapper & gl) const
	{
		assert(vsegment);
		gl.GetVertexBuffer().Draw(gl.GetActiveVertexArray(), *vsegment);
	}

	void SetVertData(const VertexBuffer::Segment & vs)
	{
		vsegment = &vs;
		enabled = (vs.vcount != 0);
	}

	RenderModelExtDrawable() :
		vsegment(NULL)
	{
		// ctor
	}

	~RenderModelExtDrawable()
	{
		// dtor
	}

private:
	const VertexBuffer::Segment * vsegment;
};

#endif
