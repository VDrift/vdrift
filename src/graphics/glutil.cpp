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

#include "glutil.h"
#include "glcore.h"
#include <ostream>

#ifdef DEBUG
bool CheckForOpenGLErrors(
	const std::string & activity_description,
	std::ostream & error_output)
{

	// glGetError stalls graphics driver, disable in release mode
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const GLubyte * error_string = glcErrorString(error);
		error_output << "OpenGL error \"" << error_string << "\" during: " << activity_description << std::endl;
		return true;
	}
	return false;
}
#else
bool CheckForOpenGLErrors(
	const std::string & /*activity_description*/,
	std::ostream & /*error_output*/)
{
	return false;
}
#endif // DEBUG
