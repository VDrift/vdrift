#ifndef _OPENGL_UTILITY_H
#define _OPENGL_UTILITY_H

#include <string>
#include <iostream>

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#else
#include <GL/glew.h>
#endif
#include <GL/gl.h>

namespace OPENGL_UTILITY
{
	/// returns true on error
	bool CheckForOpenGLErrors(std::string activity_description, std::ostream & error_output);
}

#endif
