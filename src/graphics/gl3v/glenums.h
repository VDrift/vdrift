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

#ifndef _GLENUMS_H
#define _GLENUMS_H

#include <unordered_map>
#include <string>

/// Base interface for OpenGL enumerations.
class GLEnums
{
public:
	GLEnums();

	/// Get OpenGL enumeration based on the string.
	GLenum getEnum(const std::string & value) const;

	/// Get the string based on an OpenGL enumeration.
	const std::string & getEnum(GLenum value) const;

private:
	std::unordered_map <std::string, GLenum> strToEnum;
	std::unordered_map <GLenum, std::string> enumToStr;
};

#undef ADD_GL_ENUM

#endif
