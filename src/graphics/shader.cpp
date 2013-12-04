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

#include "shader.h"
#include "utils.h"

#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>

using std::string;
using std::ostream;
using std::stringstream;
using std::ifstream;
using std::pair;
using std::endl;
using std::strcpy;

void PrintWithLineNumbers(ostream & out, const string & str)
{
	stringstream in(str);
	int count = 0;
	while (in && count < 10000)
	{
		count++;
		string linestr;
		getline(in, linestr);
		stringstream countstream;
		countstream << count;
		string countstr = countstream.str();
		out << countstr;
		for (int i = 0; i < 5 - (int)countstr.size(); i++)
			out << " ";
		out << ": " << linestr << endl;
	}
}

bool Shader::Load(const std::string & vertex_filename, const std::string & fragment_filename, const std::vector <std::string> & preprocessor_defines, std::ostream & info_output, std::ostream & error_output)
{
	assert(GLEW_ARB_shading_language_100);

	Unload();

	string vertexshader_source = Utils::LoadFileIntoString(vertex_filename, error_output);
	string fragmentshader_source = Utils::LoadFileIntoString(fragment_filename, error_output);
	assert(!vertexshader_source.empty());
	assert(!fragmentshader_source.empty());

	// prepend #define values
	for (std::vector <std::string>::const_iterator i = preprocessor_defines.begin(); i != preprocessor_defines.end(); ++i)
	{
		vertexshader_source = "#define " + *i + "\n" + vertexshader_source;
		fragmentshader_source = "#define " + *i + "\n" + fragmentshader_source;
	}

	// prepend #version
	vertexshader_source = "#version 120\n" + vertexshader_source;
	fragmentshader_source = "#version 120\n" + fragmentshader_source;

	// create shader objects
	program = glCreateProgramObjectARB();
	vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER);
	fragment_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);

	// load shader sources
	GLcharARB * vertshad = new GLcharARB[vertexshader_source.length()+1];
	strcpy(vertshad, vertexshader_source.c_str());
	const GLcharARB * vertshad2 = vertshad;
	glShaderSource(vertex_shader, 1, &vertshad2, NULL);
	delete [] vertshad;

	GLcharARB * fragshad = new GLcharARB[fragmentshader_source.length()+1];
	strcpy(fragshad, fragmentshader_source.c_str());
	const GLcharARB * fragshad2 = fragshad;
	glShaderSource(fragment_shader, 1, &fragshad2, NULL);
	delete [] fragshad;

	// compile the shaders
	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);

	GLint vertex_compiled(0);
	GLint fragment_compiled(0);

	glGetObjectParameterivARB(vertex_shader, GL_OBJECT_COMPILE_STATUS_ARB, &vertex_compiled);
	glGetObjectParameterivARB(fragment_shader, GL_OBJECT_COMPILE_STATUS_ARB, &fragment_compiled);

	if (!vertex_compiled)
		PrintShaderLog(vertex_shader, vertex_filename, error_output);

	if (!fragment_compiled)
		PrintShaderLog(fragment_shader, fragment_filename, error_output);

	// attach shader objects to the program object
	glAttachObjectARB(program, vertex_shader);
	glAttachObjectARB(program, fragment_shader);

	// link the program
	glLinkProgram(program);

	GLint program_linked(0);
	glGetProgramiv(program, GL_LINK_STATUS, &program_linked);

	if (!program_linked)
		PrintProgramLog(program, vertex_filename + " and " + fragment_filename, error_output);

	if (!(vertex_compiled && fragment_compiled && program_linked))
	{
		error_output << "Shader compilation failure: " + vertex_filename + " and " + fragment_filename << endl << endl;
		error_output << "Vertex shader:" << endl;
		PrintWithLineNumbers(error_output, vertexshader_source);
		error_output << endl;

		error_output << "Fragment shader:" << endl;
		PrintWithLineNumbers(error_output, fragmentshader_source);
		error_output << endl;

		loaded = false;
	}
	else
	{
		// need to enable to be able to set passed variable info
		glUseProgramObjectARB(program);

		// set passed variable information for tus
		for (int i = 0; i < 16; i++)
		{
			stringstream tustring;
			tustring << "tu" << i;
			int tu_loc;
			tu_loc = glGetUniformLocation(program, (tustring.str()+"_2D").c_str());
			if (tu_loc >= 0) glUniform1i(tu_loc, i);

			tu_loc = glGetUniformLocation(program, (tustring.str()+"_2DRect").c_str());
			if (tu_loc >= 0) glUniform1i(tu_loc, i);

			tu_loc = glGetUniformLocation(program, (tustring.str()+"_cube").c_str());
			if (tu_loc >= 0)
			{
				glUniform1i(tu_loc, i);
			}
		}

		loaded = true;
	}

	return loaded;
}

void Shader::Unload()
{
	if (loaded)
	{
		glDeleteObjectARB(program);
		glDeleteObjectARB(vertex_shader);
		glDeleteObjectARB(fragment_shader);
	}
	loaded = false;
}

void Shader::Enable()
{
	assert(loaded);

	glUseProgramObjectARB(program);
}

bool Shader::UploadMat16(const string & varname, float * mat16)
{
	Enable();

	int mat_loc = glGetUniformLocation(program, varname.c_str());
	if (mat_loc >= 0) glUniformMatrix4fv(mat_loc, 1, GL_FALSE, mat16);
	return (mat_loc >= 0);
}

///returns true on successful upload
bool Shader::UploadActiveShaderParameter1i(const string & param, int val)
{
	Enable();

	int loc = glGetUniformLocation(program, param.c_str());
	if (loc >= 0) glUniform1i(loc, val);
	return (loc >= 0);
}

///returns true on successful upload
bool Shader::UploadActiveShaderParameter1f(const string & param, float val)
{
	Enable();

	int loc = glGetUniformLocation(program, param.c_str());
	if (loc >= 0) glUniform1f(loc, val);
	return (loc >= 0);
}

///returns true on successful upload
bool Shader::UploadActiveShaderParameter3f(const std::string & param, float val1, float val2, float val3)
{
	Enable();

	int loc = glGetUniformLocation(program, param.c_str());
	if (loc >= 0) glUniform3f(loc, val1, val2, val3);
	return (loc >= 0);
}

///query the card for the shader program link log and print it out
void Shader::PrintProgramLog(GLhandleARB & program, const std::string & name, std::ostream & out)
{
	const unsigned int logsize = 65536;
	char shaderlog[logsize];
	GLsizei loglength;
	glGetInfoLogARB(program, logsize, &loglength, shaderlog);
	if (loglength > 0)
	{
		out << "----- Start Shader Link Log for " + name + " -----" << endl;
		out << shaderlog << endl;
		out << "----- End Shader Link Log -----" << endl;
	}
}

///query the card for the shader compile log and print it out
void Shader::PrintShaderLog(GLhandleARB & shader, const std::string & name, std::ostream & out)
{
	const unsigned int logsize = 65536;
	char shaderlog[logsize];
	GLsizei loglength;
	glGetInfoLogARB(shader, logsize, &loglength, shaderlog);
	if (loglength > 0)
	{
		out << "----- Start Shader Compile Log for " + name + " -----" << endl;
		out << shaderlog << endl;
		out << "----- End Shader Compile Log -----" << endl;
	}
}
