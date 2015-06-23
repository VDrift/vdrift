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

#include "../glcore.h"
#include "glenums.h"
#include <iostream>
#include <cassert>

GLEnums::GLEnums()
{
	#define X(x) {enumToStr[x]=#x;strToEnum[#x]=x;}
	#include "glenums.def"
	#undef X
}

GLenum GLEnums::getEnum(const std::string & value) const
{
	std::unordered_map <std::string, GLenum>::const_iterator i = strToEnum.find(value);
	if (i == strToEnum.end())
	{
		std::cerr << "Unknown GLenum string: " << value << std::endl;
		assert(0);
	}
	return i->second;
}

const std::string & GLEnums::getEnum(GLenum value) const
{
	std::unordered_map <GLenum, std::string>::const_iterator i = enumToStr.find(value);
	if (i == enumToStr.end())
	{
		std::cerr << "Unknown GLenum value: " << (int)value << std::endl;
		assert(0);
	}
	return i->second;
}

