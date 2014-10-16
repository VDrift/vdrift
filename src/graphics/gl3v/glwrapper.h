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

#ifndef _GLWRAPPER
#define _GLWRAPPER

#include "renderuniformvector.h"
#include "../glcore.h"

#include <iosfwd>
#include <string>
#include <vector>
#include <map>

class VertexBuffer;

/// A wrapper around all OpenGL functions.
/// All GL functions should go through this class only; this allows it to cache state changes and perform other optimizations.
class GLWrapper
{
public:

	GLWrapper(VertexBuffer & vb);

	/// This must be called before you can use any other functions from this class.
	/// An already created window reference must be passed in.
	/// Returns true on success.
	bool initialize();

	/// This allows specification of a stream to send error messages to a pointer to the passed value will be kept and must be valid for the lifetime of the object.
	void setErrorOutput(std::ostream & newOutput);

	/// This allows specification of a stream to send miscellaneous logging messages to a pointer to the passed value will be kept, and must be valid for the lifetime of the object.
	void setInfoOutput(std::ostream & newOutput);

	/// Calls the appropriate glUniform* function immediately.
	void applyUniform(GLint location, const RenderUniformVector <float> & data);
	void applyUniform(GLint location, const RenderUniformVector <int> & data);

	/// This checks the cache before applying the uniform.
	template <typename T>
	void applyUniformCached(GLint location, const RenderUniformVector <T> & data);

	/// This accumulates the uniform changes but delays their application until a draw call.
	/// This approach allows for cache-ing and minimization of GL calls.
	template <typename T>
	void applyUniformDelayed(GLint location, const RenderUniformVector <T> & data);

	/// Draws a vertex array object.
	void drawGeometry(GLuint vao, GLuint elementCount);

	void unbindFramebuffer();

	void unbindTexture(GLenum target);

	/// Generate a mipmap.
	void generateMipmaps(GLenum target, GLuint handle);

	/// Create and compile a shader of the given shader type.
	/// Returns true on success.
	/// Puts the generated shader handle into the provided handle variable.
	bool createAndCompileShader(const std::string & shaderSource, GLenum shaderType, GLuint & handle, std::ostream & shaderErrorOutput);

	/// Link a shader program given the specified shaders.
	/// Returns true on success.
	/// Puts the generated shader program handle into the provided handle variable.
	bool linkShaderProgram(const std::vector <std::string> & shaderAttributeBindings, const std::vector <GLuint> & shaderHandles, GLuint & handle, const std::map <GLuint, std::string> & fragDataLocations, std::ostream & shaderErrorOutput);

	/// Relinks a shader program that has previously been linked. does nothing and returns false if handle is zero.
	/// Returns true on success.
	bool relinkShaderProgram(GLuint handle, std::ostream & shaderErrorOutput);

	// A bunch of OpenGL analogues.
	// Some return bools where true means success.
	bool BindFramebuffer(GLuint fbo);
	void BindFramebufferWithoutValidation(GLuint fbo);
	void Viewport(GLuint w, GLuint h);
	void Clear(GLbitfield mask);
	void UseProgram(GLuint program);
	void Enable(GLenum cap);
	void Disable(GLenum cap);
	void Enablei(GLenum cap, GLuint index);
	void Disablei(GLenum cap, GLuint index);
	void DepthFunc(GLenum param);
	void DepthMask(GLboolean mask);
	void CullFace(GLenum param);
	void FrontFace(GLenum param);
	void PolygonMode(GLenum param);
	void PolygonOffset(GLfloat param0, GLfloat param1);
	void SampleCoverage(GLfloat param0, GLboolean param1);
	void SampleMaski(GLuint param0, GLbitfield param1);
	void Hint(GLenum param0, GLenum param1);
	void BlendEquationSeparate(GLenum param0, GLenum param1);
	void BlendFuncSeparate(GLenum param0, GLenum param1, GLenum param2, GLenum param3);
	void BindTexture(GLenum target, GLuint handle);
	void TexParameteri(GLenum target, GLenum pname, GLint param);
	void TexParameterf(GLenum target, GLenum pname, GLfloat param);
	void TexParameterfv(GLenum target, GLenum pname, const GLfloat * params);
	void SamplerParameteri(GLuint samplerHandle, GLenum pname, GLint param);
	void SamplerParameterf(GLuint samplerHandle, GLenum pname, GLfloat param);
	void SamplerParameterfv(GLuint samplerHandle, GLenum pname, const GLfloat * params);
	void ActiveTexture(unsigned int tu);
	void deleteFramebufferObject(GLuint handle);
	void deleteRenderbuffer(GLuint handle);
	void DeleteProgram(GLuint handle);
	GLuint CreateProgram();
	void DeleteShader(GLuint handle);
	GLint GetUniformLocation(GLuint shaderProgram, const std::string & uniformName);
	GLuint GenFramebuffer();
	void GetIntegerv(GLenum pname, GLint * params) const;
	void DrawBuffers(GLsizei n, const GLenum * bufs);
	GLuint GenRenderbuffer();
	void BindRenderbuffer(GLenum target,GLuint renderbuffer);
	void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void FramebufferRenderbuffer(GLenum target,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer);
	GLuint GenTexture();
	void DeleteTexture(GLuint handle);
	void TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
	void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	GLuint GenSampler();
	void DeleteSampler(GLuint handle);
	void BindSampler(GLuint unit, GLuint sampler);
	void unbindSampler(GLuint unit);
	GLuint GenVertexArray();
	void BindVertexArray(GLuint handle);
	void unbindVertexArray();
	void DeleteVertexArray(GLuint handle);
	void VertexAttribPointer(GLuint i, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
	void EnableVertexAttribArray(GLuint i);
	void DisableVertexAttribArray(GLuint i);
	void DrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices);
	void DrawArrays(GLenum mode, GLint first, GLsizei count);
	void DeleteQuery(GLuint handle);
	GLuint GenQuery();
	void BeginQuery(GLenum target, GLuint handle);
	void EndQuery(GLenum target);
	GLuint GetQueryObjectuiv(GLuint id, GLenum pname);
	void ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
	void ClearDepth(GLfloat d);
	void ClearStencil(GLint s);

	VertexBuffer & GetVertexBuffer() { return vertexBuffer; }
	unsigned int & GetActiveVertexArray() { return curActiveVertexArray; }

	/// Writes errors to the log.
	/// Returns false if there was an error.
	bool checkForOpenGLErrors(const char * function, const char * file, int line) const;

	/// Enable or disable logging.
	void logging(bool log);

private:
	VertexBuffer & vertexBuffer;
	unsigned int curActiveVertexArray;

	std::ostream * infoOutput;
	std::ostream * errorOutput;
	bool initialized;
	bool logEnable; // Only does anything if logEveryGlCall in glwrapper.cpp is true.

	// Cached state.
	unsigned int curActiveTexture;
	std::vector <GLuint> boundTextures;
	std::vector <RenderUniformVector<float> > cachedUniformFloats; // indexed by location
	std::vector <RenderUniformVector<int> > cachedUniformInts; // indexed by location
	std::vector <unsigned int> cachedUniformFloatsToApplyNextDrawCall; // indexed by location
	std::vector <unsigned int> cachedUniformIntsToApplyNextDrawCall; // indexed by location

	void clearCaches();

	/// Notifies the uniform cache of new data.
	/// Returns true if the new data matches the old data.
	template <typename T>
	bool uniformCache(GLint location, const RenderUniformVector <T> & data, std::vector <RenderUniformVector<T> > & cache);
	bool uniformCache(GLint location, const RenderUniformVector <float> & data);
	bool uniformCache(GLint location, const RenderUniformVector <int> & data);

	// Logging.
	void logError(const std::string & msg) const;
	void logOutput(const std::string & msg) const;
	void logGlCall(const char * msg) const;
};

#undef ERROR_CHECK
#undef ERROR_CHECK1
#undef ERROR_CHECK2
#undef GLLOG
#undef CACHED

#endif
