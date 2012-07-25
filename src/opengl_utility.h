#ifndef _OPENGL_UTILITY_H
#define _OPENGL_UTILITY_H

#include <string>
#include <iostream>

namespace OPENGL_UTILITY
{
	/// returns true on error
	bool CheckForOpenGLErrors(
		const std::string & activity_description,
		std::ostream & error_output);
}

#endif
