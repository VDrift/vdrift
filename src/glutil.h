#ifndef _GLUTIL_H
#define _GLUTIL_H

#include <string>
#include <iostream>

namespace GLUTIL
{
	/// returns true on error
	bool CheckForOpenGLErrors(
		const std::string & activity_description,
		std::ostream & error_output);
}

#endif
