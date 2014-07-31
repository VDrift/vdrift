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

#ifndef _VERTEX_FORMAT_H
#define _VERTEX_FORMAT_H

#include "vertexattrib.h"

struct VertexFormat
{
	VertexAttrib::Format attribs[VertexAttrib::LastAttrib + 1];
	unsigned int attribs_count;
	unsigned int stride;

	/// predefined vertex formats
	enum Enum
	{
		PNT332,
		PTC324,
		PT32,
		P3,
		LastFormat = P3
	};
	static const VertexFormat & Get(Enum e);
};

#endif // _VERTEX_FORMAT_H
