#ifndef _OPENGL_UTILITY_H
#define _OPENGL_UTILITY_H

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <string>
#include <iostream>

namespace OPENGL_UTILITY
{
	/// returns true on error
	bool CheckForOpenGLErrors(std::string activity_description, std::ostream & error_output);
}

#endif
