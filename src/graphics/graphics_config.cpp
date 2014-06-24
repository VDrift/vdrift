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

#include "graphics_config.h"

#include <sstream>
#include <fstream>
#include <cassert>
#include <map>

std::string strLTrim(std::string instr)
{
	return instr.erase(0, instr.find_first_not_of(" \t\r\377"));
}

std::string strRTrim(std::string instr)
{
	if (instr.find_last_not_of(" \t\r\377") != std::string::npos)
		return instr.erase(instr.find_last_not_of(" \t\r\377") + 1);
	else
		return instr;
}

std::string strTrim(std::string instr)
{
	return strLTrim(strRTrim(instr));
}

std::string getTrimmedLine(std::istream & in)
{
	std::string linestr;
	std::getline(in, linestr);
	return strTrim(linestr);
}

std::string peekTrimmedLine(std::istream & in)
{
	std::streampos pos = in.tellg();
	std::string line = getTrimmedLine(in);
	in.seekg(pos);
	return line;
}

const char comment = ';';

/// returns the skipped-over cruft
std::string skip(std::istream & in, const std::string & skipchars, int * linecount = NULL)
{
	std::string cruft;
	char next = in.peek();
	while (in && skipchars.find(next) != std::string::npos)
	{
		if (next == comment)
		{
			getTrimmedLine(in); //discard comment
			if (linecount)
				(*linecount)++;
			next = in.peek();
		}
		else
		{
			cruft.push_back(in.get());
			if (*cruft.rend() == '\n' && linecount)
				(*linecount)++;
			next = in.peek();
		}
	}
	return cruft;
}

std::string skipWS(std::istream & in, int * linecount = NULL)
{
	return skip(in, " \t\r\n", linecount);
}

/// returns the skipped-over cruft
std::string skipUntil(std::istream & in, const std::string & skipchars, int * linecount = NULL)
{
	std::string cruft;
	char next = in.peek();
	while (in && skipchars.find(next) == std::string::npos)
	{
		if (next == comment)
		{
			getTrimmedLine(in); // discard comment
			if (linecount)
				(*linecount)++;
			next = in.peek();
		}
		else
		{
			cruft.push_back(in.get());
			if (*cruft.rbegin() == '\n' && linecount) // last character a newline
				(*linecount)++;
			next = in.peek();
		}
	}
	return cruft;
}

// reads lines as value=whatever pairs until the end of the file or until a line that begins with [ is encountered
std::map <std::string, std::string> readValuePairs(std::istream & f, int & line)
{
	const char terminator = '[';

	std::map <std::string, std::string> varmap;

	while (f && (peekTrimmedLine(f).empty() || peekTrimmedLine(f)[0] != terminator))
	{
		std::istringstream lineparse(getTrimmedLine(f));
		line++;

		std::string name = strTrim(skipUntil(lineparse, "="));
		if (lineparse)
		{
			char separator = lineparse.get();
			assert(separator == '=');
			std::string value = strTrim(skipUntil(lineparse, ";"));
			varmap[name] = value;
		}
	}

	return varmap;
}

bool readSection(std::istream & f, std::ostream & error_output, int & line, const std::vector <std::string> & reqd, std::map <std::string, std::string> & outputmap)
{
	int sectionline = line;
	outputmap = readValuePairs(f, line);
	for (std::vector <std::string>::const_iterator i = reqd.begin(); i != reqd.end(); i++)
	{
		if (outputmap[*i].empty())
		{
			error_output << "Variable \"" << *i << "\" is required in the section starting at line " << sectionline << std::endl;
			return false;
		}
	}

	return true;
}

#define ASSIGNVAR(x) x = vars[#x]
#define ASSIGNPARSE(x) x.Parse(vars[#x])
#define ASSIGNOTHER(x) {std::istringstream defparser(vars[#x]);defparser >> x;}
#define ASSIGNBOOL(x) {x = (vars[#x] == "true");}

bool isOf(const std::string & val, const std::string & list, std::ostream * error_output, int line)
{
	std::istringstream parser(list);
	while (parser)
	{
		std::string item;
		parser >> item;
		if (val == item)
			return true;
	}

	if (error_output)
	{
		*error_output << "Expected one of these values: \"" << list << "\" but got value \"" << val << "\" in section starting on line " << line << std::endl;
	}
	return false;
}

bool isOf(std::map <std::string, std::string> & vars, const std::string & name, const std::string & list, std::ostream * error_output, int line)
{
	return isOf(vars[name], list, error_output, line);
}

bool GraphicsConfigShader::Load(std::istream & f, std::ostream & error_output, int & line)
{
	std::vector <std::string> reqd;
	reqd.push_back("name");
	reqd.push_back("fragment");
	reqd.push_back("vertex");

	int sectionstart = line;

	std::map <std::string, std::string> vars;
	if (!readSection(f, error_output, line, reqd, vars))
		return false;

	for (std::map <std::string, std::string>::const_iterator i = vars.begin(); i != vars.end(); i++)
	{
		if (!isOf(i->first, "name fragment vertex defines", &error_output, sectionstart)) return false;
	}

	ASSIGNVAR(name);
	ASSIGNVAR(fragment);
	ASSIGNVAR(vertex);
	ASSIGNVAR(defines);

	return true;
}

void GraphicsConfigOutput::Size::Parse(const std::string & str)
{
	std::istringstream parser(str);
	std::string lhs = strTrim(skipUntil(parser, "/*"));
	bool mult = false;
	bool div = false;
	std::string rhs;
	if (parser)
	{
		char next = parser.get();
		if (next == '/')
			div = true;
		else if (next == '*')
			mult = true;
		else
		{
			assert(0);
		}
		rhs = getTrimmedLine(parser);
	}

	value = 0;
	fb_div = 0;
	fb_mult = 0;
	if (lhs == "framebuffer")
	{
		if (mult || div)
		{
			std::istringstream parser2(rhs);
			if (div)
				parser2 >> fb_div;
			else
				parser2 >> fb_mult;
		}
	}
	else
	{
		std::istringstream parser2(lhs);
		parser2 >> value;
	}
}

void fillDefault(std::map <std::string, std::string> & vars, const std::string & name, const std::string & value)
{
	if (vars[name].empty())
		vars[name] = value;
}

bool GraphicsConfigOutput::Load(std::istream & f, std::ostream & error_output, int & line)
{
	int sectionstart = line;

	std::vector <std::string> reqd;
	reqd.push_back("name");
	//reqd.push_back("width");
	//reqd.push_back("height");
	reqd.push_back("type");
	//reqd.push_back("format");
	//reqd.push_back("filter");
	//reqd.push_back("mipmap");
	//reqd.push_back("multisample");

	std::map <std::string, std::string> vars;
	if (!readSection(f, error_output, line, reqd, vars))
		return false;

	for (std::map <std::string, std::string>::const_iterator i = vars.begin(); i != vars.end(); i++)
	{
		if (!isOf(i->first, "name width height type filter format mipmap multisample conditions", &error_output, sectionstart)) return false;
	}

	// fill in defaults
	fillDefault(vars, "filter", "linear");
	fillDefault(vars, "mipmap", "false");
	fillDefault(vars, "multisample", "0");
	fillDefault(vars, "width", "framebuffer");
	fillDefault(vars, "height", "framebuffer");
	fillDefault(vars, "format", "RGB8");
	if (vars["multisample"] == "framebuffer")
		vars["multisample"] = "-1";

	ASSIGNVAR(name);
	ASSIGNPARSE(width);
	ASSIGNPARSE(height);
	ASSIGNVAR(type);
	if (!isOf(vars, "type", "2D rectangle cube framebuffer", &error_output, sectionstart)) return false;
	ASSIGNVAR(filter);
	if (!isOf(vars, "filter", "linear nearest", &error_output, sectionstart)) return false;
	ASSIGNVAR(format);
	if (!isOf(vars, "format", "R8 RGB8 RGBA8 RGB16 RGBA16 depthshadow depth", &error_output, sectionstart)) return false;
	ASSIGNBOOL(mipmap);
	ASSIGNOTHER(multisample);
	ASSIGNPARSE(conditions);

	return true;
}

void GraphicsConfigInputs::Parse(const std::string & str)
{
	int tucount = 0;
	std::istringstream parser(str);
	while (parser)
	{
		std::string raw;
		parser >> raw;
		raw = strTrim(raw);
		size_t pos = raw.find(':');
		if (pos == std::string::npos)
		{
			// no colon
			if (!raw.empty())
			{
				tu[tucount] = raw;
				tucount++;
			}
		}
		else
		{
			// colon
			std::istringstream parser2(raw.substr(0, pos));
			parser2 >> tucount;
			std::string texname = raw.substr(pos+1);
			if (!texname.empty())
			{
				tu[tucount] = raw.substr(pos+1);
				tucount++;
			}
		}
	}
}

bool GraphicsConfigPass::Load(std::istream & f, std::ostream & error_output, int & line)
{
	int sectionstart = line;

	std::vector <std::string> reqd;
	//reqd.push_back("camera");
	reqd.push_back("draw");
	//reqd.push_back("light");
	reqd.push_back("output");
	reqd.push_back("shader");

	std::map <std::string, std::string> vars;
	if (!readSection(f, error_output, line, reqd, vars))
		return false;

	for (std::map <std::string, std::string>::const_iterator i = vars.begin(); i != vars.end(); i++)
	{
		if (!isOf(i->first, "draw camera light output shader inputs clear_color clear_depth write_color write_alpha write_depth cull conditions blendmode depthtest", &error_output, sectionstart)) return false;
	}

	bool postprocess = (vars["draw"] == "postprocess");

	// fill in defaults
	fillDefault(vars, "shader", "NO SHADER SPECIFIED");
	fillDefault(vars, "light", "sun");
	fillDefault(vars, "clear_color", "false");
	fillDefault(vars, "clear_depth", "false");
	fillDefault(vars, "write_color", "true");
	fillDefault(vars, "write_alpha", "true");
	fillDefault(vars, "write_depth", postprocess ? "false" : "true");
	fillDefault(vars, "cull", "true");
	fillDefault(vars, "camera", "default");
	fillDefault(vars, "blendmode", postprocess ? "disabled" : "disabled");
	fillDefault(vars, "depthtest", postprocess ? "disabled" : "lequal");

	std::string blendmodes = "disabled alphablend add alphablend_premultiplied";

	if (!isOf(vars, "blendmode", blendmodes, &error_output, sectionstart)) return false;
	if (!isOf(vars, "depthtest", "lequal equal gequal disabled", &error_output, sectionstart)) return false;


	// process draw as a comma delimited list
	{
		if (postprocess)
			draw.push_back("postprocess");
		else
		{
			std::istringstream parser(vars["draw"]);
			std::string layer;
			while (parser >> layer)
			{
				if (layer == "postprocess")
				{
					error_output << "Error: postprocess must be the only item in the draw list in the section starting on line " << sectionstart << std::endl;
					return false;
				}
				if (!layer.empty())
					draw.push_back(layer);
			}
		}
	}

	ASSIGNVAR(camera);
	ASSIGNVAR(light);
	ASSIGNVAR(output);
	ASSIGNVAR(shader);
	ASSIGNPARSE(inputs);
	ASSIGNBOOL(clear_color);
	ASSIGNBOOL(clear_depth);
	ASSIGNBOOL(write_color);
	ASSIGNBOOL(write_alpha);
	ASSIGNBOOL(write_depth);
	ASSIGNBOOL(cull);
	ASSIGNPARSE(conditions);
	ASSIGNVAR(blendmode);
	ASSIGNVAR(depthtest);

	return true;
}

bool GraphicsConfig::Load(const std::string & filename, std::ostream & error_output)
{
	std::ifstream f(filename.c_str(), std::ifstream::binary); // binary mode to avoid newline/seekg issues
	if (!f)
	{
		error_output << "Unable to open graphics config file: " << filename << std::endl;
		return false;
	}
	return Load(f, error_output);
}

bool GraphicsConfig::Load(std::istream & f, std::ostream & error_output)
{
	int line = 1;

	// find the first section
	skipUntil(f, "[", &line);

	while (f)
	{
		// read the section title
		char bracket = f.get();
		assert(bracket == '[');
		std::string type = strTrim(skipUntil(f, "]", &line));
		if (!f)
		{
			error_output << "Unexpected end of file while reading section header on line " << line << std::endl;
			return false;
		}

		// consume the rest of the line
		getTrimmedLine(f);
		line++;

		// handle the section
		if (type == "shader")
		{
			GraphicsConfigShader newsection;
			if (!newsection.Load(f, error_output, line))
				return false;
			shaders.push_back(newsection);
		}
		else if (type == "output")
		{
			GraphicsConfigOutput newsection;
			if (!newsection.Load(f, error_output, line))
				return false;
			outputs.push_back(newsection);
		}
		else if (type == "pass")
		{
			GraphicsConfigPass newsection;
			if (!newsection.Load(f, error_output, line))
				return false;
			passes.push_back(newsection);
		}
		else
		{
			error_output << "Unexpected section header type \"" << type << "\" on line " << line << std::endl;
			return false;
		}

		// find the next section
		skipUntil(f, "[", &line);
	}

	return true;
}

#undef ASSIGNVAR
#undef ASSIGNPARSE
#undef ASSIGNOTHER
#undef ASSIGNBOOL
