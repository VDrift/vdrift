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

#include "renderuniformentry.h"
#include "stringidmap.h"

RenderUniformBase::RenderUniformBase()
{
	// Constructor.
}

RenderUniformBase::RenderUniformBase(const float * newData, int dataSize) : data(newData, dataSize)
{
	// Constructor.
}

RenderUniformBase::RenderUniformBase(const std::vector <float> & newdata) : data(newdata)
{
	// Constructor.
}

RenderUniformBase::RenderUniformBase(const RenderUniformVector <float> & newdata) : data(newdata)
{
	// Constructor.
}

RenderUniformEntry::RenderUniformEntry()
{
	// Constructor.
}

RenderUniformEntry::RenderUniformEntry(StringId newName, const float * newData, int dataSize) : RenderUniformBase(newData, dataSize), name(newName)
{
	// Constructor.
}
