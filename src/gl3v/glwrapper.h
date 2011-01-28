#ifndef _GLWRAPPER
#define _GLWRAPPER

#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GL/gl.h>

#include "utils.h"

#define ERROR_CHECK checkForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+UTILS::tostr(__LINE__))
#define ERROR_CHECK1(x) checkForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+UTILS::tostr(__LINE__)+" arg1="+UTILS::tostr(x))
#define ERROR_CHECK2(x1,x2) checkForOpenGLErrors(std::string(__PRETTY_FUNCTION__)+":"+__FILE__+":"+UTILS::tostr(__LINE__)+" arg1="+UTILS::tostr(x1)+",arg2="+UTILS::tostr(x2))
#define GLLOG(x) (logGlCall(#x),x)

/// a wrapper around all OpenGL functions
/// all GL functions should go through this class only; this allows it to cache state changes
/// and perform other optimizations
class GLWrapper
{
	public:
		/// returns true on success
		/// this must be called before you can use any other functions from this class
		/// an already created window reference must be passed in
		bool initialize();
		
		/// this allows specification of a stream to send error messages to
		/// a pointer to the passed value will be kept, and must be valid
		/// for the lifetime of the object
		void setErrorOutput(std::ostream & newOutput) {errorOutput = &newOutput;}
		
		/// this allows specification of a stream to send miscellaneous logging 
		/// messages to
		/// a pointer to the passed value will be kept, and must be valid
		/// for the lifetime of the object
		void setInfoOutput(std::ostream & newOutput) {infoOutput = &newOutput;}
		
		/// calls the appropriate glUniform* function
		void applyUniform(GLint location, const std::vector <float> & data);
		void applyUniform(GLint location, const std::vector <int> & data);
		
		/// draws a vertex array object
		void drawGeometry(GLuint vao, GLuint elementCount)
		{
			GLLOG(glBindVertexArray(vao));ERROR_CHECK1(vao);
			GLLOG(glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, 0));ERROR_CHECK2(vao,elementCount);
			/*logOutput("drawGeometry "+UTILS::tostr(vao)+","+UTILS::tostr(elementCount));*/
		}
		
		void unbindFramebuffer() {GLLOG(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));ERROR_CHECK;}
		
		void unbindTexture(GLenum target) {GLLOG(glBindTexture(target, 0));ERROR_CHECK;}
		
		/// generate a mipmap
		void generateMipmaps(GLenum target, GLuint handle)
		{
			BindTexture(target, handle);ERROR_CHECK;
			GLLOG(glGenerateMipmap(target));ERROR_CHECK;
			BindTexture(target, 0);ERROR_CHECK;
		}
		
		/// create and compile a shader of the given shader type.
		/// returns true on success.
		/// puts the generated shader handle into the provided handle variable.
		bool createAndCompileShader(const std::string & shaderSource, GLenum shaderType, GLuint & handle, std::ostream & shaderErrorOutput);
		
		/// link a shader program given the specified shaders.
		/// returns true on success.
		/// puts the generated shader program handle into the provided handle variable.
		bool linkShaderProgram(const std::vector <std::string> & shaderAttributeBindings, const std::vector <GLuint> & shaderHandles, GLuint & handle, std::ostream & shaderErrorOutput);
		
		// a bunch of OpenGL analogues; some return bools where true means success
		bool BindFramebuffer(GLuint fbo);
		void BindFramebufferWithoutValidation(GLuint fbo) {GLLOG(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));ERROR_CHECK;}
		void Viewport(GLuint w, GLuint h) {GLLOG(glViewport(0,0,w,h));ERROR_CHECK;}
		void Clear(GLbitfield mask) {GLLOG(glClear(mask));ERROR_CHECK;}
		void UseProgram(GLuint program) {GLLOG(glUseProgram(program));ERROR_CHECK;}
		void Enable(GLenum cap) {GLLOG(glEnable(cap));ERROR_CHECK;}
		void Disable(GLenum cap) {GLLOG(glDisable(cap));ERROR_CHECK;}
		void Enablei(GLenum cap, GLuint index) {GLLOG(glEnablei(cap,index));ERROR_CHECK;}
		void Disablei(GLenum cap, GLuint index) {GLLOG(glDisablei(cap,index));ERROR_CHECK;}
		void DepthFunc(GLenum param) {GLLOG(glDepthFunc(param));ERROR_CHECK;}
		void CullFace(GLenum param) {GLLOG(glCullFace(param));ERROR_CHECK;}
		void FrontFace(GLenum param) {GLLOG(glFrontFace(param));ERROR_CHECK;}
		void PolygonMode(GLenum param) {GLLOG(glPolygonMode(GL_FRONT_AND_BACK, param));ERROR_CHECK;}
		void PolygonOffset(GLfloat param0, GLfloat param1) {GLLOG(glPolygonOffset(param0, param1));ERROR_CHECK;}
		void SampleCoverage(GLfloat param0, GLboolean param1) {GLLOG(glSampleCoverage(param0, param1));ERROR_CHECK;}
		void SampleMaski(GLuint param0, GLbitfield param1) {GLLOG(glSampleMaski(param0, param1));ERROR_CHECK;}
		void Hint(GLenum param0, GLenum param1) {GLLOG(glHint(param0, param1));ERROR_CHECK;}
		void BlendEquationSeparate(GLenum param0, GLenum param1) {GLLOG(glBlendEquationSeparate(param0,param1));ERROR_CHECK;}
		void BlendFuncSeparate(GLenum param0, GLenum param1, GLenum param2, GLenum param3) {GLLOG(glBlendFuncSeparate(param0, param1, param2, param3));ERROR_CHECK;}
		void BindTexture(GLenum target, GLuint handle) {GLLOG(glBindTexture(target,handle));/*logOutput("BindTexture "+UTILS::tostr(handle));*/ERROR_CHECK2(target,handle);}
		void TexParameteri(GLenum target, GLenum pname, GLint param) {GLLOG(glTexParameteri(target, pname, param));ERROR_CHECK;}
		void TexParameterf(GLenum target, GLenum pname, GLfloat param)  {GLLOG(glTexParameterf(target, pname, param));ERROR_CHECK;}
		void TexParameterfv(GLenum target, GLenum pname, const GLfloat * params)  {GLLOG(glTexParameterfv(target, pname, params));ERROR_CHECK;}
		void SamplerParameteri(GLuint samplerHandle, GLenum pname, GLint param) {GLLOG(glSamplerParameteri(samplerHandle, pname, param));ERROR_CHECK;}
		void SamplerParameterf(GLuint samplerHandle, GLenum pname, GLfloat param)  {GLLOG(glSamplerParameterf(samplerHandle, pname, param));ERROR_CHECK;}
		void SamplerParameterfv(GLuint samplerHandle, GLenum pname, const GLfloat * params)  {GLLOG(glSamplerParameterfv(samplerHandle, pname, params));ERROR_CHECK;}
		void ActiveTexture(unsigned int tu) {GLLOG(glActiveTexture(GL_TEXTURE0+tu));ERROR_CHECK;}
		void deleteFramebufferObject(GLuint handle) {GLLOG(glDeleteFramebuffers(1, &handle));ERROR_CHECK;}
		void deleteRenderbuffer(GLuint handle) {GLLOG(glDeleteRenderbuffers(1, &handle));ERROR_CHECK;}
		void DeleteProgram(GLuint handle) {GLLOG(glDeleteProgram(handle));ERROR_CHECK;}
		GLuint CreateProgram() {GLuint result = GLLOG(glCreateProgram());ERROR_CHECK;return result;}
		void DeleteShader(GLuint handle) {GLLOG(glDeleteShader(handle));ERROR_CHECK;}
		GLint GetUniformLocation(GLuint shaderProgram, const std::string & uniformName) {GLuint result = GLLOG(glGetUniformLocation(shaderProgram, uniformName.c_str()));ERROR_CHECK;return result;}
		GLuint GenFramebuffer() {GLuint result(0);GLLOG(glGenFramebuffers(1,&result));ERROR_CHECK;return result;}
		void GetIntegerv(GLenum pname, GLint * params) const {GLLOG(glGetIntegerv(pname, params));ERROR_CHECK;}
		void DrawBuffers(GLsizei n, const GLenum * bufs) {GLLOG(glDrawBuffers(n, bufs));ERROR_CHECK;}
		GLuint GenRenderbuffer() {GLuint result;GLLOG(glGenRenderbuffers(1, &result));ERROR_CHECK;return result;}
		void BindRenderbuffer(GLenum target,GLuint renderbuffer) {GLLOG(glBindRenderbuffer(target, renderbuffer));ERROR_CHECK;}
		void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {GLLOG(glRenderbufferStorage(target, internalformat, width, height));ERROR_CHECK;}
		void FramebufferRenderbuffer(GLenum target,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer) {GLLOG(glFramebufferRenderbuffer(target,attachment,renderbuffertarget,renderbuffer));ERROR_CHECK;}
		GLuint GenTexture() {GLuint result;GLLOG(glGenTextures(1, &result));ERROR_CHECK;return result;}
		void DeleteTexture(GLuint handle) {GLLOG(glDeleteTextures(1, &handle));ERROR_CHECK;}
		void TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data) {GLLOG(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));ERROR_CHECK;}
		void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {GLLOG(glFramebufferTexture2D(target, attachment, textarget, texture, level));ERROR_CHECK;}
		GLuint GenSampler() {GLuint result;GLLOG(glGenSamplers(1, &result));ERROR_CHECK;return result;}
		void DeleteSampler(GLuint handle) {GLLOG(glDeleteSamplers(1, &handle));ERROR_CHECK;}
		void BindSampler(GLuint unit, GLuint sampler) {GLLOG(glBindSampler(unit,sampler));ERROR_CHECK2(unit,sampler);}
		void unbindSampler(GLuint unit) {GLLOG(glBindSampler(unit,0));ERROR_CHECK;}
		GLuint GenVertexArray() {GLuint result;GLLOG(glGenVertexArrays(1, &result));ERROR_CHECK;return result;}
		void BindVertexArray(GLuint handle) {GLLOG(glBindVertexArray(handle));ERROR_CHECK;}
		void DeleteVertexArray(GLuint handle) {GLLOG(glDeleteVertexArrays(1, &handle));ERROR_CHECK;}
		void VertexAttribPointer(GLuint i, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer){GLLOG(glVertexAttribPointer(i, size, type, normalized, stride, pointer));ERROR_CHECK;}
		void EnableVertexAttribArray(GLuint i){GLLOG(glEnableVertexAttribArray(i));ERROR_CHECK;}
		void DisableVertexAttribArray(GLuint i){GLLOG(glDisableVertexAttribArray(i));ERROR_CHECK;}
		void DrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices) {GLLOG(glDrawElements(mode, count, type, indices));ERROR_CHECK;}
		
		/// writes errors to the log
		/// returns false if there was an error
		bool checkForOpenGLErrors(std::string activity_description) const;
		
		GLWrapper() : initialized(false), infoOutput(NULL), errorOutput(NULL) {}
		
	private:
		bool initialized;
		std::ostream * infoOutput;
		std::ostream * errorOutput;
		
		void logError(const std::string & msg) const;
		void logOutput(const std::string & msg) const;
		void logGlCall(const std::string & msg) const;
		
		// cached state
};

#undef ERROR_CHECK
#undef ERROR_CHECK1
#undef ERROR_CHECK2
#undef GLLOG

#endif