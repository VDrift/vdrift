#include "glwrapper.h"

#include "glenums.h"
#include "utils.h"

#define ERROR_CHECK checkForOpenGLErrors(__PRETTY_FUNCTION__,__FILE__,__LINE__)
#define GLLOG(x) (logGlCall(#x),x)

#define breakOnError true
#define logEveryGlCall false
#define enableErrorChecking true

const char * REQUIRED_GL_VERSION = "GL_VERSION_3_3";
const GLEnums GLEnumHelper;

void GLWrapper::logGlCall(const char * msg) const
{
	if (logEveryGlCall && logEnable)
		logOutput(msg);
}

bool GLWrapper::checkForOpenGLErrors(const char * function, const char * file, int line) const
{
	if (enableErrorChecking)
	{
		GLenum gl_error = glGetError();
		if (gl_error != GL_NO_ERROR)
		{
			const GLubyte *err_string = gluErrorString(gl_error);
			std::string activity_description = std::string(function)+":"+file+":"+UTILS::tostr(line);
			logError(std::string("OpenGL error \"")+UTILS::tostr(err_string)+"\" during: "+activity_description);
			assert(!breakOnError);
			return false;
		}
	}
	return true;
}

bool GLWrapper::initialize()
{
	logOutput(std::string("GL Renderer: ")+UTILS::tostr(glGetString(GL_RENDERER)));
	logOutput(std::string("GL Vendor: ")+UTILS::tostr(glGetString(GL_VENDOR)));
	logOutput(std::string("GL Version: ")+UTILS::tostr(glGetString(GL_VERSION)));

	GLenum glew_err = glewInit();
	if (glew_err != GLEW_OK)
	{
		logError(std::string("GLEW failed to initialize: ")+UTILS::tostr(glewGetErrorString(glew_err)));
		return false;
	}
	else
	{
		logOutput(std::string("Initialized GLEW ")+UTILS::tostr(glewGetString(GLEW_VERSION)));
	}

	// check through all OpenGL versions to determine the highest supported OpenGL version
	bool supportsRequiredVersion = glewIsSupported(REQUIRED_GL_VERSION);

	if (!supportsRequiredVersion)
	{
		logError(std::string("Graphics card or driver does not support required ")+REQUIRED_GL_VERSION);
		return false;
	}

	return ERROR_CHECK;
}

void GLWrapper::logError(const std::string & msg) const
{
	if (errorOutput)
		(*errorOutput) << msg << std::endl;
}

void GLWrapper::logOutput(const std::string & msg) const
{
	if (infoOutput)
		(*infoOutput) << msg << std::endl;
}

bool GLWrapper::BindFramebuffer(GLuint fbo)
{
	GLLOG(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));ERROR_CHECK;
	GLenum status = GLLOG(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));ERROR_CHECK;
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		logError("Incomplete framebuffer: "+GLEnumHelper.getEnum(status));
		return false;
	}
	else
		return true;
}

void GLWrapper::applyUniform(GLint location, const RenderUniformVector <float> & data)
{
	switch (data.size())
	{
		case 1:
		GLLOG(glUniform1f(location, data[0]));ERROR_CHECK;
		break;

		case 2:
		GLLOG(glUniform2f(location, data[0], data[1]));ERROR_CHECK;
		break;

		case 3:
		GLLOG(glUniform3f(location, data[0], data[1], data[2]));ERROR_CHECK;
		break;

		case 4:
		GLLOG(glUniform4f(location, data[0], data[1], data[2], data[3]));ERROR_CHECK;
		break;

		case 16:
		GLLOG(glUniformMatrix4fv(location, 1, false, &data[0]));ERROR_CHECK;
		break;

		default:
		logError("Encountered unexpected uniform size: " + UTILS::tostr(data.size()) + " location " +UTILS::tostr(location));
		assert(!"unexpected uniform size");
	};
}

void GLWrapper::applyUniform(GLint location, const RenderUniformVector <int> & data)
{
	switch (data.size())
	{
		case 1:
		GLLOG(glUniform1i(location, data[0]));ERROR_CHECK;
		break;

		case 2:
		GLLOG(glUniform2i(location, data[0], data[1]));ERROR_CHECK;
		break;

		case 3:
		GLLOG(glUniform3i(location, data[0], data[1], data[2]));ERROR_CHECK;
		break;

		case 4:
		GLLOG(glUniform4i(location, data[0], data[1], data[2], data[3]));ERROR_CHECK;
		break;

		default:
		logError("Encountered unexpected uniform size: " + UTILS::tostr(data.size()) + " location " +UTILS::tostr(location));
		assert(0 && "unexpected uniform size");
	};
}

bool GLWrapper::createAndCompileShader(const std::string & shaderSource, GLenum shaderType, GLuint & handle, std::ostream & shaderErrorOutput)
{
	const GLchar * shaderSourcePointer = shaderSource.c_str();
	handle = GLLOG(glCreateShader(shaderType));ERROR_CHECK;
	GLLOG(glShaderSource(handle, 1, &shaderSourcePointer, NULL));ERROR_CHECK;
	GLLOG(glCompileShader(handle));ERROR_CHECK;
	GLint compileStatus(0);
	GLLOG(glGetShaderiv(handle, GL_COMPILE_STATUS, &compileStatus));ERROR_CHECK;

	GLint bufferSize(0);
	GLLOG(glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &bufferSize));ERROR_CHECK;
	GLchar* infoLog = new GLchar[bufferSize+1];
	GLsizei infoLogLength;
	GLLOG(glGetShaderInfoLog(handle, bufferSize, &infoLogLength, infoLog));ERROR_CHECK;
	infoLog[bufferSize] = '\0';
	shaderErrorOutput << infoLog;
	delete[] infoLog;

	if (!compileStatus)
	{
		GLLOG(glDeleteShader(handle));ERROR_CHECK;
		handle = 0;
		return false;
	}
	else
	{
		return true;
	}
}

bool GLWrapper::linkShaderProgram(const std::vector <std::string> & shaderAttributeBindings, const std::vector <GLuint> & shaderHandles, GLuint & handle, const std::map <GLuint, std::string> & fragDataLocations, std::ostream & shaderErrorOutput)
{
	handle = GLLOG(glCreateProgram());ERROR_CHECK;

	// attach all shaders that we got (hopefully a vertex and fragment shader are in here)
	for (unsigned int i = 0; i < shaderHandles.size(); i++)
	{
		GLLOG(glAttachShader(handle, shaderHandles[i]));ERROR_CHECK;
	}

	// make sure we get our vertex attributes bound to the proper names
	for (unsigned int i = 0; i < shaderAttributeBindings.size(); i++)
	{
		if (!shaderAttributeBindings[i].empty())
		{
			GLLOG(glBindAttribLocation(handle, i, shaderAttributeBindings[i].c_str()));ERROR_CHECK;
		}
	}

	// make sure color outputs are bound to the proper names
	for (std::map <GLuint, std::string>::const_iterator i = fragDataLocations.begin(); i != fragDataLocations.end(); i++)
	{
		GLLOG(glBindFragDataLocation(handle, i->first, i->second.c_str()));ERROR_CHECK;
	}

	// attempt to link the program
	GLLOG(glLinkProgram(handle));ERROR_CHECK;

	// handle the result
	GLint linkStatus;
	GLLOG(glGetProgramiv(handle, GL_LINK_STATUS, &linkStatus));
	if (!linkStatus)
	{
		GLint bufferSize(0);
		GLLOG(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &bufferSize));
		GLchar* infoLog = new GLchar[bufferSize+1];
		GLsizei infoLogLength;
		GLLOG(glGetProgramInfoLog(handle, bufferSize, &infoLogLength, infoLog));
		infoLog[bufferSize] = '\0';
		shaderErrorOutput << "Linking of shader program failed:\n" << infoLog << std::endl;
		GLLOG(glDeleteProgram(handle));ERROR_CHECK;
		delete[] infoLog;
		handle = 0;
		return false;
	}
	else
	{
		return true;
	}
}

bool GLWrapper::relinkShaderProgram(GLuint handle, std::ostream & shaderErrorOutput)
{
	if (!handle)
		return false;

	// attempt to link the program
	GLLOG(glLinkProgram(handle));ERROR_CHECK;

	// handle the result
	GLint linkStatus;
	GLLOG(glGetProgramiv(handle, GL_LINK_STATUS, &linkStatus));
	if (!linkStatus)
	{
		GLint bufferSize(0);
		GLLOG(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &bufferSize));
		GLchar* infoLog = new GLchar[bufferSize+1];
		GLsizei infoLogLength;
		GLLOG(glGetProgramInfoLog(handle, bufferSize, &infoLogLength, infoLog));
		infoLog[bufferSize] = '\0';
		shaderErrorOutput << "Linking of shader program failed:\n" << infoLog << std::endl;
		GLLOG(glDeleteProgram(handle));ERROR_CHECK;
		delete[] infoLog;
		return false;
	}
	else
	{
		return true;
	}
}

void GLWrapper::BindTexture(GLenum target, GLuint handle)
{
	// only cache 2D textures at the moment, so if it's not 2D, then just send it and return
	// if we don't know what TU is active, then we can't do cache either
	if (target != GL_TEXTURE_2D || curActiveTexture == UINT_MAX)
	{
		GLLOG(glBindTexture(target,handle));ERROR_CHECK;
		return;
	}

	// check the cache
	bool send = false;
	if (curActiveTexture < boundTextures.size())
	{
		if (boundTextures[curActiveTexture] != handle)
			send = true;
	}
	else
	{
		boundTextures.resize(curActiveTexture+1,0);
		send = true;
	}

	if (send)
	{
		GLLOG(glBindTexture(target,handle));ERROR_CHECK;
		boundTextures[curActiveTexture] = handle;
	}
}
