#include "opengl_utility.h"

#include <cassert>

bool OPENGL_UTILITY::CheckForOpenGLErrors(std::string activity_description, std::ostream & error_output)
{
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
	|| defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
	//disable error checking because intel has many driver chipsets out with non-compliant OpenGL drivers
#else
	GLenum gl_error = glGetError();
	if (gl_error != GL_NO_ERROR)
	{
		const GLubyte *err_string = gluErrorString(gl_error);
		error_output << "OpenGL error \"" << err_string << "\" during: " << activity_description << std::endl;
		//assert (gl_error == GL_NO_ERROR);
		return true;
	}
#endif
	return false;
}
