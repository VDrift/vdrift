#include "graphics_config.h"

#include <sstream>
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
				*linecount++;
			next = in.peek();
		}
		else
		{
			cruft.push_back(in.get());
			if (*cruft.rend() == '\n' && linecount)
				*linecount++;
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
				*linecount++;
			next = in.peek();
		}
		else
		{
			cruft.push_back(in.get());
			if (*cruft.rend() == '\n' && linecount)
				*linecount++;
			next = in.peek();
		}
	}
	return cruft;
}

// reads lines as value=whatever pairs until the end of the file or until a line that begins with [ is encountered
std::map <std::string, std::string> readValuePairs(std::istream & f, std::ostream & error_output, int & line)
{
	const char terminator = '[';
	
	std::map <std::string, std::string> varmap;
	
	while (f && (peekTrimmedLine(f).empty() || peekTrimmedLine(f)[0] != terminator))
	{
		std::stringstream lineparse(getTrimmedLine(f));
		line++;
		
		std::string name = strTrim(skipUntil(lineparse, "="));
		if (lineparse)
		{
			assert(lineparse.get() == '=');
			std::string value = strTrim(skipUntil(lineparse, ";"));
			varmap[name] = value;
		}
	}
	
	return varmap;
}

bool readSection(std::istream & f, std::ostream & error_output, int & line, const std::vector <std::string> & reqd, std::map <std::string, std::string> & outputmap)
{
	int sectionline = line;
	outputmap = readValuePairs(f, error_output, line);
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
#define ASSIGNOTHER(x) {std::stringstream defparser(vars[#x]);defparser >> x;}
#define ASSIGNBOOL(x) {x = (vars[#x] == "true");}

bool GRAPHICS_CONFIG_SHADER::Load(std::istream & f, std::ostream & error_output, int & line)
{
	std::vector <std::string> reqd;
	reqd.push_back("name");
	
	std::map <std::string, std::string> vars;
	if (!readSection(f, error_output, line, reqd, vars))
		return false;
	
	// fill in defaults
	if (vars["folder"].empty())
		vars["folder"] = vars["name"];
	
	ASSIGNVAR(name);
	ASSIGNVAR(folder);
	ASSIGNVAR(defines);
	
	return true;
}

void GRAPHICS_CONFIG_OUTPUT::SIZE::Parse(const std::string & str)
{
	std::stringstream parser(str);
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
			std::stringstream parser2(rhs);
			if (div)
				parser2 >> fb_div;
			else
				parser2 >> fb_mult;
		}
	}
	else
	{
		std::stringstream parser2(lhs);
		parser2 >> value;
	}
}

void fillDefault(std::map <std::string, std::string> & vars, const std::string & name, const std::string & value)
{
	if (vars[name].empty())
		vars[name] = value;
}

void GRAPHICS_CONFIG_CONDITION::Parse(const std::string & str)
{
	std::stringstream parser(str);
	while (parser)
	{
		std::string condition;
		parser >> condition;
		
		if (!condition.empty())
		{
			if (condition[0] == '!')
			{
				if (condition.size() > 1)
				{
					std::string cond = condition.substr(1);
					if (positive_conditions.find(cond) == positive_conditions.end())
						negated_conditions.insert(cond);
					else
						positive_conditions.erase(cond);
				}
			}
			else
			{
				if (negated_conditions.find(condition) == negated_conditions.end())
					positive_conditions.insert(condition);
				else
					negated_conditions.erase(condition);
			}
		}
	}
}

bool GRAPHICS_CONFIG_CONDITION::Satisfied(const std::set <std::string> & conditions) const
{
	for (std::set <std::string>::const_iterator i = positive_conditions.begin(); i != positive_conditions.end(); i++)
	{
		if (conditions.find(*i) == conditions.end())
		{
			return false;
		}
	}
	
	for (std::set <std::string>::const_iterator i = negated_conditions.begin(); i != negated_conditions.end(); i++)
	{
		if (conditions.find(*i) != conditions.end())
		{
			return false;
		}
	}
	
	return true;
}

bool isOf(std::map <std::string, std::string> & vars, const std::string & name, const std::string & list, std::ostream * error_output, int line)
{
	std::stringstream parser(list);
	std::string val = strTrim(vars[name]);
	while (parser)
	{
		std::string item;
		parser >> item;
		if (val == item)
			return true;
	}
	
	if (error_output)
	{
		*error_output << "Expected variable \"" << name << "\" to have one of these values: \"" << list << "\" but it had value \"" << val << "\" in section starting on line " << line << std::endl;
	}
	return false;
}

bool GRAPHICS_CONFIG_OUTPUT::Load(std::istream & f, std::ostream & error_output, int & line)
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
	
	// fill in defaults
	fillDefault(vars, "filter", "linear");
	fillDefault(vars, "mipmap", "false");
	fillDefault(vars, "multisample", "0");
	fillDefault(vars, "width", "framebuffer");
	fillDefault(vars, "height", "framebuffer");
	fillDefault(vars, "format", "RGB");
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
	if (!isOf(vars, "format", "RGB RGBA depth", &error_output, sectionstart)) return false;
	ASSIGNBOOL(mipmap);
	ASSIGNOTHER(multisample);
	ASSIGNPARSE(conditions);
	
	return true;
}

void GRAPHICS_CONFIG_INPUTS::Parse(const std::string & str)
{
	int tucount = 0;
	std::stringstream parser(str);
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
			std::stringstream parser2(raw.substr(0, pos));
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

bool GRAPHICS_CONFIG_PASS::Load(std::istream & f, std::ostream & error_output, int & line)
{
	std::vector <std::string> reqd;
	//reqd.push_back("camera");
	reqd.push_back("draw");
	//reqd.push_back("light");
	reqd.push_back("output");
	reqd.push_back("shader");
	
	std::map <std::string, std::string> vars;
	if (!readSection(f, error_output, line, reqd, vars))
		return false;
	
	// fill in defaults
	fillDefault(vars, "light", "sun");
	fillDefault(vars, "clear_color", "false");
	fillDefault(vars, "clear_depth", "false");
	fillDefault(vars, "write_depth", "true");
	fillDefault(vars, "cull", "true");
	fillDefault(vars, "camera", "default");
	
	ASSIGNVAR(camera);
	ASSIGNVAR(draw);
	ASSIGNVAR(light);
	ASSIGNVAR(output);
	ASSIGNVAR(shader);
	ASSIGNPARSE(inputs);
	ASSIGNBOOL(clear_color);
	ASSIGNBOOL(clear_depth);
	ASSIGNBOOL(write_depth);
	ASSIGNBOOL(cull);
	ASSIGNPARSE(conditions);
	
	return true;
}

bool GRAPHICS_CONFIG::Load(std::istream & f, std::ostream & error_output)
{
	int line = 1;
	
	// find the first section
	skipUntil(f, "[", &line);
	
	while (f)
	{
		// read the section title
		assert(f.get() == '[');
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
			GRAPHICS_CONFIG_SHADER newsection;
			if (!newsection.Load(f, error_output, line))
				return false;
			shaders.push_back(newsection);
		}
		else if (type == "output")
		{
			GRAPHICS_CONFIG_OUTPUT newsection;
			if (!newsection.Load(f, error_output, line))
				return false;
			outputs.push_back(newsection);
		}
		else if (type == "pass")
		{
			GRAPHICS_CONFIG_PASS newsection;
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