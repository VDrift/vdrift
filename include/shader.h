#ifndef _SHADER_H
#define _SHADER_H

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif

#include <string>
#include <iostream>
#include <vector>

class SHADER_GLSL
{
private:
	bool loaded;
	GLhandleARB program;
	GLhandleARB vertex_shader;
	GLhandleARB fragment_shader;

	void PrintShaderLog(GLhandleARB & shader, const std::string & name, std::ostream & out);
	void PrintProgramLog(GLhandleARB & program, const std::string & name, std::ostream & out);

public:
	SHADER_GLSL() : loaded(false) {}
	~SHADER_GLSL() {Unload();}

	void Unload();
	bool Load(const std::string & vertex_filename, const std::string & fragment_filename, const std::vector <std::string> & preprocessor_defines, std::ostream & info_output, std::ostream & error_output);
	void Enable();
	void EndScene();
	bool UploadMat16(const std::string & varname, float * mat16);
	bool UploadActiveShaderParameter1i(const std::string & param, int val);
	bool UploadActiveShaderParameter1f(const std::string & param, float val);
	bool UploadActiveShaderParameter3f(const std::string & param, float val1, float val2, float val3);
	template <typename T>
	bool UploadActiveShaderParameter3f(const std::string & param, T vector)
	{
		return UploadActiveShaderParameter3f(param, vector[0], vector[1], vector[2]);
	}
	bool GetLoaded() const {return loaded;}
};

#endif
