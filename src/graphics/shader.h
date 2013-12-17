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

class Shader
{
public:
	Shader();

	~Shader();

	void Unload();

	bool Load(
		const std::string & vertex_filename,
		const std::string & fragment_filename,
		const std::vector<std::string> & defines,
		const std::vector<std::string> & uniforms,
		std::ostream & info_output,
		std::ostream & error_output);

	bool GetLoaded() const;

	void Enable();

	///< allocate uniform slot and get uniform location
	///< doesn't check for duplicates
	int RegisterUniform(const char name[]);

	///< id is the uniform name index in the uniform list passed during shader loading
	///< Enable() shader before using SetUniform calls
	bool SetUniformMat4f(int id, const float val[16]);

	bool SetUniformMat3f(int id, const float val[9]);

	bool SetUniform1i(int id, int val);

	bool SetUniform1f(int id, float val);

	bool SetUniform3f(int id, float val1, float val2, float val3);

	template <typename T>
	bool SetUniform3f(int id, T vec)
	{
		return SetUniform3f(id, vec[0], vec[1], vec[2]);
	}

private:
	GLhandleARB program;
	GLhandleARB vertex_shader;
	GLhandleARB fragment_shader;
	std::vector <int> uniform_locations;

	void PrintShaderLog(GLhandleARB & shader, const std::string & name, std::ostream & out);

	void PrintProgramLog(GLhandleARB & program, const std::string & name, std::ostream & out);
};

#endif
