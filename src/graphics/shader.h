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

#ifndef _SHADER_H
#define _SHADER_H

#include "glew.h"
#include <iosfwd>
#include <string>
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
