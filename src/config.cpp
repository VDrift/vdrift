#include "config.h"
#include "unittest.h"

#include <fstream>

static std::string LTrim(std::string instr)
{
	return instr.erase(0, instr.find_first_not_of(" \t"));
}

static std::string RTrim(std::string instr)
{
	if (instr.find_last_not_of(" \t") != std::string::npos)
		return instr.erase(instr.find_last_not_of(" \t") + 1);
	else
		return instr;
}

static std::string Trim(std::string instr)
{
	return LTrim(RTrim(instr));
}

static std::string Strip(std::string instr, char stripchar)
{
	std::string::size_type pos = 0;
	std::string outstr = "";
	
	while (pos < instr.length())
	{
		if (instr.c_str()[pos] != stripchar)
		{
			outstr = outstr + instr.substr(pos, 1);
		}
		pos++;
	}
	
	return outstr;
}

static std::string GetAbsolutePath(std::string dir, std::string relpath)
{
	std::string::size_type i, j;
	while ((i = relpath.find("../")) == 0)
	{
		relpath.erase(0, 3);
		j = dir.rfind('/');
		if (j != std::string::npos) dir = dir.substr(0, j);
	}
	return dir + "/" + relpath;
}

CONFIG::CONFIG()
{
	SUPPRESS_ERROR = false;
}

CONFIG::CONFIG(std::string fname)
{

	SUPPRESS_ERROR = false;
	Load(fname);
}

CONFIG::~CONFIG()
{
	Clear();
}

void CONFIG::Clear()
{
	filename.clear();
	sections.clear();
}

bool CONFIG::Load(std::string fname)
{
	if (filename.length() == 0)
	{
		filename = fname;
	}
	else if (include.find(fname) != include.end())
	{
		return true;
	}
	include.insert(fname);
	
	std::ifstream f(fname.c_str());
	if (!f && !SUPPRESS_ERROR)
	{
		return false;
	}
	
	return Load(f);
}

bool CONFIG::Load(std::istream & f)
{
	// create a base section (unnamed)
	CONFIG::iterator section = sections.insert(std::pair<std::string, SECTION>("", SECTION())).first;
	
	std::string line;
	while (f && !f.eof())
	{
		std::getline(f, line, '\n');
		ProcessLine(section, line);
	}
	
	//DebugPrint(std::cerr);
	
	return true;
}

void CONFIG::ProcessLine(CONFIG::iterator & section, std::string & linestr)
{
	linestr = Trim(linestr);
	linestr = Strip(linestr, '\r');
	linestr = Strip(linestr, '\n');
	
	//remove comments
	std::string::size_type commentpos = linestr.find("#", 0);
	if (commentpos < linestr.length())
	{
		linestr = linestr.substr(0, commentpos);
	}
	
	linestr = Trim(linestr);
	
	//only continue if not a blank line or comment-only line
	if (linestr.length() > 0)
	{
		if (linestr.find("=", 0) < linestr.length())
		{
			//find the name part
			std::string::size_type equalpos = linestr.find("=", 0);
			std::string name = linestr.substr(0, equalpos);
			equalpos++;
			std::string val = linestr.substr(equalpos, linestr.length() - equalpos);
			name = Trim(name);
			val = Trim(val);
			
			//only continue if valid
			if (name.length() > 0 && val.length() > 0)
			{
				section->second[name] = val;
			}
		}
		else if(linestr.find("include ") == 0)
		{
			//configfile include
			std::string dir;
			std::string relpath = linestr.erase(0, 8);
			std::string::size_type pos = filename.rfind('/');
			if (pos != std::string::npos) dir = filename.substr(0, pos);
			std::string path = GetAbsolutePath(dir, relpath);
			Load(path);
		}
		else
		{
			//section header
			linestr = Strip(linestr, '[');
			linestr = Strip(linestr, ']');
			linestr = Trim(linestr);
			
			// subsection
			size_t n = linestr.rfind('/');
			if (n != std::string::npos)
			{
				std::string parent = linestr.substr(0, n);
				std::string child = linestr.substr(n+1);
				sections[parent][child] = linestr;
			}
			section = sections.insert(std::pair<std::string, SECTION>(linestr, SECTION())).first;
		}
	}
}

void CONFIG::DebugPrint(std::ostream & out, bool with_brackets) const
{
	for (const_iterator im = sections.begin(); im != sections.end(); ++im)
	{
		if (!im->first.empty())
		{
			std::string sectionname = im->first;
			if (with_brackets) out << "[" << sectionname << "]" << std::endl;
			else out << sectionname << std::endl;
		}
		for (SECTION::const_iterator is = im->second.begin(); is != im->second.end(); ++is)
		{
			out << is->first << " = " << is->second << std::endl;
		}
		out << std::endl;
	}
}

bool CONFIG::Write(bool with_brackets) const
{
	return Write(with_brackets, filename);
}

bool CONFIG::Write(bool with_brackets, std::string save_as) const
{
	std::ofstream f(save_as.c_str());
	if (!f) return false;
	
	DebugPrint(f);
	
	return true;
}

QT_TEST(configfile_test)
{
	std::stringstream instream;
	instream << 
		"\n#comment on the FIRST LINE??\n\n"
		"variable outside of=a section\n\n"
		"test section numero UNO\n"
		"look at me = 23.4\n\n"
		"i'm so great=   BANANA\n"
		"#break!\n\n"
		"[ section    dos??]\n"
		"why won't you = breeeak #trying to break it\n\n"
		"what about ] # this malformed thing???\n"
		"nope works = fine.\n"
		"even vectors = 2.1,0.9,GAMMA\n"
		"this is a duplicate = 0\n"
		"this is a duplicate = 1\n"
		"random = intermediary\n"
		"this is a duplicate = 2\n";
	
	CONFIG testconfig;
	testconfig.Load(instream);
	std::string tstr = "notfound";
	QT_CHECK(testconfig.GetParam("", "variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam("section    dos??", "why won't you", tstr));
	QT_CHECK_EQUAL(tstr, "breeeak");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam("", "variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(!testconfig.GetParam("nosection", "novariable", tstr));
	QT_CHECK_EQUAL(tstr, "notfound");
	tstr = "notfound";
	std::vector<float> vec(3);
	QT_CHECK(testconfig.GetParam("what about", "even vectors", vec));
	QT_CHECK_EQUAL(vec[0], 2.1f);
	QT_CHECK_EQUAL(vec[1], 0.9f);
	QT_CHECK_EQUAL(vec[2], 0.f);
	//testconfig.DebugPrint(std::cout);
	
	{
		CONFIG::const_iterator i = testconfig.begin();
		QT_CHECK_EQUAL(i->first, "");
		i++;
		QT_CHECK_EQUAL(i->first, "section    dos??");
		i++;
		QT_CHECK_EQUAL(i->first, "test section numero UNO");
		i++;
		QT_CHECK_EQUAL(i->first, "what about");
		i++;
		QT_CHECK(i == testconfig.end());
	}
	
	{
		std::string value;
		CONFIG::const_iterator i;
		testconfig.GetSection("test section numero UNO", i);
		QT_CHECK_EQUAL(testconfig.GetParam(i, "i'm so great", value), true);
		QT_CHECK_EQUAL(testconfig.GetParam(i, "look at me", value), true);
	}
}

