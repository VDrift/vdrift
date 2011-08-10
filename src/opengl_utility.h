#ifndef _OPENGL_UTILITY_H
#define _OPENGL_UTILITY_H

#include <string>
#include <iostream>

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

namespace OPENGL_UTILITY
{
	/// returns true on error
	bool CheckForOpenGLErrors(std::string activity_description, std::ostream & error_output);
}

#endif
