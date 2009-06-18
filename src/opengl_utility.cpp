#include "opengl_utility.h"

void OPENGL_UTILITY::CheckForOpenGLErrors(std::string activity_description, std::ostream & error_output)
{
	GLenum gl_error = glGetError();
	if (gl_error != GL_NO_ERROR)
	{
		const GLubyte *err_string = gluErrorString(gl_error);
		error_output << "OpenGL error \"" << err_string << "\" during: " << activity_description << std::endl;
		assert (gl_error == GL_NO_ERROR);
	}
}
