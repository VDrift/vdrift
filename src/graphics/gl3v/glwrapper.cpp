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

#include "glwrapper.h"
#include "glenums.h"
#include "utils.h"
#include <limits.h>

#define ERROR_CHECK checkForOpenGLErrors(__PRETTY_FUNCTION__,__FILE__,__LINE__)
#define ERROR_CHECK1(x) checkForOpenGLErrors(__PRETTY_FUNCTION__,__FILE__,__LINE__)
#define ERROR_CHECK2(x1,x2) checkForOpenGLErrors(__PRETTY_FUNCTION__,__FILE__,__LINE__)
#define GLLOG(x) (logGlCall(#x),x)
#define CACHED(cachedValue,newValue,statement) {if (cachedValue != newValue) {cachedValue = newValue;statement}}

#define breakOnError false
#define logEveryGlCall false

#ifdef DEBUG
#define enableErrorChecking true
#else
#define enableErrorChecking false
#endif

const GLEnums GLEnumHelper;

GLWrapper::GLWrapper(VertexBuffer & vb) :
	vertexBuffer(vb),
	curActiveVertexArray(0),
	infoOutput(NULL),
	errorOutput(NULL),
	initialized(false),
	logEnable(false)
{
	clearCaches();
}

bool GLWrapper::initialize()
{
	int major_version = 0;
	int minor_version = 0;
	const GLubyte * version = glGetString(GL_VERSION);
	if (version[0] > '2')
	{
		glGetIntegerv(GL_MAJOR_VERSION, &major_version);ERROR_CHECK;
		glGetIntegerv(GL_MINOR_VERSION, &minor_version);ERROR_CHECK;
	}
	if (major_version < 3 || (major_version == 3 && minor_version < 3))
	{
		logError("Graphics card or driver does not support required GL Version: 3.3");
		return false;
	}
	return true;
}

void GLWrapper::setErrorOutput(std::ostream & newOutput)
{
	errorOutput = &newOutput;
}

void GLWrapper::setInfoOutput(std::ostream & newOutput)
{
	infoOutput = &newOutput;
}

void GLWrapper::applyUniform(GLint location, const RenderUniformVector <float> & data)
{
	switch(data.size())
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
		logError("Encountered unexpected uniform size: " + Utils::tostr(data.size()) + " location " +Utils::tostr(location));
		assert(!"unexpected uniform size");
	};
}

void GLWrapper::applyUniform(GLint location, const RenderUniformVector <int> & data)
{
	switch(data.size())
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
		logError("Encountered unexpected uniform size: " + Utils::tostr(data.size()) + " location " +Utils::tostr(location));
		assert(0 && "unexpected uniform size");
	};
}

template <typename T>
void GLWrapper::applyUniformCached(GLint location, const RenderUniformVector <T> & data)
{
	if (!uniformCache(location, data))
		applyUniform(location, data);
}

template <typename T>
void GLWrapper::applyUniformDelayed(GLint location, const RenderUniformVector <T> & data)
{
	if (!uniformCache(location, data))
		applyUniform(location, data);
}

void GLWrapper::drawGeometry(GLuint vao, GLuint elementCount)
{
	curActiveVertexArray = vao;
	GLLOG(glBindVertexArray(vao));ERROR_CHECK1(vao);
	GLLOG(glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0));ERROR_CHECK2(vao,elementCount);
}

void GLWrapper::unbindFramebuffer()
{
	GLLOG(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));ERROR_CHECK;
}

void GLWrapper::unbindTexture(GLenum target)
{
	BindTexture(target,0);
}

void GLWrapper::generateMipmaps(GLenum target, GLuint handle)
{
	BindTexture(target, handle);ERROR_CHECK;
	GLLOG(glGenerateMipmap(target));ERROR_CHECK;
	unbindTexture(target);ERROR_CHECK;
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
		return true;
}

bool GLWrapper::linkShaderProgram(const std::vector <std::string> & shaderAttributeBindings, const std::vector <GLuint> & shaderHandles, GLuint & handle, const std::map <GLuint, std::string> & fragDataLocations, std::ostream & shaderErrorOutput)
{
	handle = GLLOG(glCreateProgram());ERROR_CHECK;

	// Attach all shaders that we got (hopefully a vertex and fragment shader are in here).
	for (unsigned int i = 0; i < shaderHandles.size(); i++)
		GLLOG(glAttachShader(handle, shaderHandles[i]));ERROR_CHECK;

	// Make sure we get our vertex attributes bound to the proper names.
	for (unsigned int i = 0; i < shaderAttributeBindings.size(); i++)
		if (!shaderAttributeBindings[i].empty())
			GLLOG(glBindAttribLocation(handle, i, shaderAttributeBindings[i].c_str()));ERROR_CHECK;

	// Make sure color outputs are bound to the proper names.
	for (std::map <GLuint, std::string>::const_iterator i = fragDataLocations.begin(); i != fragDataLocations.end(); i++)
		GLLOG(glBindFragDataLocation(handle, i->first, i->second.c_str()));ERROR_CHECK;

	// Attempt to link the program.
	GLLOG(glLinkProgram(handle));ERROR_CHECK;

	// Handle the result.
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
		return true;
}

bool GLWrapper::relinkShaderProgram(GLuint handle, std::ostream & shaderErrorOutput)
{
	if (!handle)
		return false;

	// Attempt to link the program.
	GLLOG(glLinkProgram(handle));ERROR_CHECK;

	// Handle the result.
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
		return true;
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

void GLWrapper::BindFramebufferWithoutValidation(GLuint fbo)
{
	GLLOG(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));ERROR_CHECK;
}

void GLWrapper::Viewport(GLuint w, GLuint h)
{
	GLLOG(glViewport(0,0,w,h));ERROR_CHECK;
}

void GLWrapper::Clear(GLbitfield mask)
{
	GLLOG(glClear(mask));ERROR_CHECK;
}

void GLWrapper::UseProgram(GLuint program)
{
	GLLOG(glUseProgram(program));ERROR_CHECK;
	clearCaches();
}

void GLWrapper::Enable(GLenum cap)
{
	GLLOG(glEnable(cap));ERROR_CHECK;
}

void GLWrapper::Disable(GLenum cap)
{
	GLLOG(glDisable(cap));ERROR_CHECK;
}

void GLWrapper::Enablei(GLenum cap, GLuint index)
{
	GLLOG(glEnablei(cap,index));ERROR_CHECK;
}

void GLWrapper::Disablei(GLenum cap, GLuint index)
{
	GLLOG(glDisablei(cap,index));ERROR_CHECK;
}

void GLWrapper::DepthFunc(GLenum param)
{
	GLLOG(glDepthFunc(param));ERROR_CHECK;
}

void GLWrapper::DepthMask(GLboolean mask)
{
	GLLOG(glDepthMask(mask));ERROR_CHECK;
}

void GLWrapper::CullFace(GLenum param)
{
	GLLOG(glCullFace(param));ERROR_CHECK;
}

void GLWrapper::FrontFace(GLenum param)
{
	GLLOG(glFrontFace(param));ERROR_CHECK;
}

void GLWrapper::PolygonMode(GLenum param)
{
	GLLOG(glPolygonMode(GL_FRONT_AND_BACK, param));ERROR_CHECK;
}

void GLWrapper::PolygonOffset(GLfloat param0, GLfloat param1)
{
	GLLOG(glPolygonOffset(param0, param1));ERROR_CHECK;
}

void GLWrapper::SampleCoverage(GLfloat param0, GLboolean param1)
{
	GLLOG(glSampleCoverage(param0, param1));ERROR_CHECK;
}

void GLWrapper::SampleMaski(GLuint param0, GLbitfield param1)
{
	GLLOG(glSampleMaski(param0, param1));ERROR_CHECK;
}

void GLWrapper::Hint(GLenum param0, GLenum param1)
{
	GLLOG(glHint(param0, param1));ERROR_CHECK;
}

void GLWrapper::BlendEquationSeparate(GLenum param0, GLenum param1)
{
	GLLOG(glBlendEquationSeparate(param0,param1));ERROR_CHECK;
}

void GLWrapper::BlendFuncSeparate(GLenum param0, GLenum param1, GLenum param2, GLenum param3)
{
	GLLOG(glBlendFuncSeparate(param0, param1, param2, param3));ERROR_CHECK;
}

void GLWrapper::BindTexture(GLenum target, GLuint handle)
{
	// Only cache 2D textures at the moment, so if it's not 2D, then just send it and return.
	// If we don't know what TU is active, then we can't do cache either.
	if (target != GL_TEXTURE_2D || curActiveTexture == UINT_MAX)
	{
		GLLOG(glBindTexture(target,handle));ERROR_CHECK;
		return;
	}

	// Check the cache.
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

void GLWrapper::TexParameteri(GLenum target, GLenum pname, GLint param)
{
	GLLOG(glTexParameteri(target, pname, param));ERROR_CHECK;
}

void GLWrapper::TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	GLLOG(glTexParameterf(target, pname, param));ERROR_CHECK;
}

void GLWrapper::TexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
	GLLOG(glTexParameterfv(target, pname, params));ERROR_CHECK;
}

void GLWrapper::SamplerParameteri(GLuint samplerHandle, GLenum pname, GLint param)
{
	GLLOG(glSamplerParameteri(samplerHandle, pname, param));ERROR_CHECK;
}

void GLWrapper::SamplerParameterf(GLuint samplerHandle, GLenum pname, GLfloat param)
{
	GLLOG(glSamplerParameterf(samplerHandle, pname, param));ERROR_CHECK;
}

void GLWrapper::SamplerParameterfv(GLuint samplerHandle, GLenum pname, const GLfloat * params)
{
	GLLOG(glSamplerParameterfv(samplerHandle, pname, params));ERROR_CHECK;
}

void GLWrapper::ActiveTexture(unsigned int tu)
{
	CACHED(curActiveTexture,tu,GLLOG(glActiveTexture(GL_TEXTURE0+tu));ERROR_CHECK;)
}

void GLWrapper::deleteFramebufferObject(GLuint handle)
{
	GLLOG(glDeleteFramebuffers(1, &handle));ERROR_CHECK;
}

void GLWrapper::deleteRenderbuffer(GLuint handle)
{
	GLLOG(glDeleteRenderbuffers(1, &handle));ERROR_CHECK;
}

void GLWrapper::DeleteProgram(GLuint handle)
{
	GLLOG(glDeleteProgram(handle));ERROR_CHECK;
}

GLuint GLWrapper::CreateProgram()
{
	GLuint result = GLLOG(glCreateProgram());ERROR_CHECK;
	return result;
}

void GLWrapper::DeleteShader(GLuint handle)
{
	GLLOG(glDeleteShader(handle));ERROR_CHECK;
}

GLint GLWrapper::GetUniformLocation(GLuint shaderProgram, const std::string & uniformName)
{
	GLuint result = GLLOG(glGetUniformLocation(shaderProgram, uniformName.c_str()));ERROR_CHECK;
	return result;
}

GLuint GLWrapper::GenFramebuffer()
{
	GLuint result(0);
	GLLOG(glGenFramebuffers(1,&result));ERROR_CHECK;
	return result;
}

void GLWrapper::GetIntegerv(GLenum pname, GLint * params) const
{
	GLLOG(glGetIntegerv(pname, params));ERROR_CHECK;
}

void GLWrapper::DrawBuffers(GLsizei n, const GLenum * bufs)
{
	GLLOG(glDrawBuffers(n, bufs));ERROR_CHECK;
}

GLuint GLWrapper::GenRenderbuffer()
{
	GLuint result;
	GLLOG(glGenRenderbuffers(1, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::BindRenderbuffer(GLenum target,GLuint renderbuffer)
{
	GLLOG(glBindRenderbuffer(target, renderbuffer));ERROR_CHECK;
}

void GLWrapper::RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	GLLOG(glRenderbufferStorage(target, internalformat, width, height));ERROR_CHECK;
}

void GLWrapper::FramebufferRenderbuffer(GLenum target,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer)
{
	GLLOG(glFramebufferRenderbuffer(target,attachment,renderbuffertarget,renderbuffer));ERROR_CHECK;
}

GLuint GLWrapper::GenTexture()
{
	GLuint result;
	GLLOG(glGenTextures(1, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::DeleteTexture(GLuint handle)
{
	GLLOG(glDeleteTextures(1, &handle));ERROR_CHECK;
}

void GLWrapper::TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data)
{
	GLLOG(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));ERROR_CHECK;
}

void GLWrapper::FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	GLLOG(glFramebufferTexture2D(target, attachment, textarget, texture, level));ERROR_CHECK;
}

GLuint GLWrapper::GenSampler() {
	GLuint result;
	GLLOG(glGenSamplers(1, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::DeleteSampler(GLuint handle)
{
	GLLOG(glDeleteSamplers(1, &handle));ERROR_CHECK;
}

void GLWrapper::BindSampler(GLuint unit, GLuint sampler)
{
	GLLOG(glBindSampler(unit,sampler));ERROR_CHECK2(unit,sampler);
}

void GLWrapper::unbindSampler(GLuint unit)
{
	GLLOG(glBindSampler(unit,0));ERROR_CHECK;
}

GLuint GLWrapper::GenVertexArray()
{
	GLuint result;
	GLLOG(glGenVertexArrays(1, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::BindVertexArray(GLuint handle)
{
	curActiveVertexArray = handle;
	GLLOG(glBindVertexArray(handle));ERROR_CHECK;
}

void GLWrapper::unbindVertexArray()
{
	curActiveVertexArray = 0;
	GLLOG(glBindVertexArray(0));ERROR_CHECK;
}

void GLWrapper::DeleteVertexArray(GLuint handle)
{
	GLLOG(glDeleteVertexArrays(1, &handle));ERROR_CHECK;
}

void GLWrapper::VertexAttribPointer(GLuint i, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)
{
	GLLOG(glVertexAttribPointer(i, size, type, normalized, stride, pointer));ERROR_CHECK;
}

void GLWrapper::EnableVertexAttribArray(GLuint i)
{
	GLLOG(glEnableVertexAttribArray(i));ERROR_CHECK;
}

void GLWrapper::DisableVertexAttribArray(GLuint i)
{
	GLLOG(glDisableVertexAttribArray(i));ERROR_CHECK;
}

void GLWrapper::DrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices)
{
	GLLOG(glDrawElements(mode, count, type, indices));ERROR_CHECK;
}

void GLWrapper::DrawArrays(GLenum mode, GLint first, GLsizei count)
{
	GLLOG(glDrawArrays(mode, first, count));ERROR_CHECK;
}

void GLWrapper::DeleteQuery(GLuint handle)
{
	GLLOG(glDeleteQueries(1, &handle));ERROR_CHECK;
}

GLuint GLWrapper::GenQuery()
{
	GLuint result;
	GLLOG(glGenQueries(1, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::BeginQuery(GLenum target, GLuint handle)
{
	GLLOG(glBeginQuery(target, handle));ERROR_CHECK;
}

void GLWrapper::EndQuery(GLenum target)
{
	GLLOG(glEndQuery(target));ERROR_CHECK;
}

GLuint GLWrapper::GetQueryObjectuiv(GLuint id, GLenum pname)
{
	GLuint result;
	GLLOG(glGetQueryObjectuiv(id, pname, &result));ERROR_CHECK;
	return result;
}

void GLWrapper::ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	GLLOG(glClearColor(r,g,b,a));ERROR_CHECK;
}

void GLWrapper::ClearDepth(GLfloat d)
{
	GLLOG(glClearDepth(d));ERROR_CHECK;
}

void GLWrapper::ClearStencil(GLint s)
{
	GLLOG(glClearStencil(s));ERROR_CHECK;
}

bool GLWrapper::checkForOpenGLErrors(const char * function, const char * file, int line) const
{
	if (enableErrorChecking)
	{
		GLenum gl_error = glGetError();
		if (gl_error != GL_NO_ERROR)
		{
			const GLubyte *err_string = glcErrorString(gl_error);
			std::string activity_description = std::string(function)+":"+file+":"+Utils::tostr(line);
			logError(std::string("OpenGL error \"")+Utils::tostr(err_string)+"\" during: "+activity_description);
			assert(!breakOnError);
			return false;
		}
	}
	return true;
}

void GLWrapper::logging(bool log)
{
	logEnable = log;
}

void GLWrapper::clearCaches()
{
	curActiveTexture = UINT_MAX;
	boundTextures.clear();
	cachedUniformFloats.clear();
	cachedUniformInts.clear();
}

template <typename T>
bool GLWrapper::uniformCache(GLint location, const RenderUniformVector <T> & data, std::vector <RenderUniformVector<T> > & cache)
{
	bool match = true;

	// If we don't have an entry for this location yet, it's definitely not a match.
	if (location >= (int)cache.size())
	{
		match = false;
		cache.resize(location+1);
	}

	// Look up the cached value.
	RenderUniformVector<T> & cached = cache[location];

	// Check if the sizes of the vectors match.
	if (cached.size() != data.size())
		match = false;

	// If we've passed tests so far, check the values themselves.
	if (match && (std::memcmp(&cached[0],&data[0],data.size()*sizeof(T)) != 0))
		match = false;

	// Update the cached value if it has changed.
	if (!match)
		cached = data;

	return match;
}

bool GLWrapper::uniformCache(GLint location, const RenderUniformVector <float> & data)
{
	return uniformCache(location, data, cachedUniformFloats);
}

bool GLWrapper::uniformCache(GLint location, const RenderUniformVector <int> & data)
{
	return uniformCache(location, data, cachedUniformInts);
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

void GLWrapper::logGlCall(const char * msg) const
{
	if (logEveryGlCall && logEnable)
		logOutput(msg);
}

#undef ERROR_CHECK
#undef ERROR_CHECK1
#undef ERROR_CHECK2
#undef GLLOG
#undef CACHED
