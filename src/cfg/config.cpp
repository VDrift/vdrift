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

Config::Config() :
	SUPPRESS_ERROR(false)
{
	// ctor
}

Config::Config(const std::string & fname) :
	SUPPRESS_ERROR(false)
{
	load(fname);
}

Config::~Config()
{
	clear();
}

void Config::clear()
{
	filename.clear();
	sections.clear();
}

bool Config::load(const std::string & fname)
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
	return load(f);
}

bool Config::load(std::istream & f)
{
	if (!SUPPRESS_ERROR && (!f || f.eof()))
	{
		return false;
	}

	// strip UTF-8 BOM
	if (f.get() != 0xEF || f.get() != 0xBB || f.get() != 0xBF)
	{
		f.seekg(0);
	}

	// create empty section
	iterator section = sections.insert(std::pair<std::string, Section>("", Section())).first;

	// start parsing
	std::string line;
	while (f && !f.eof())
	{
		std::getline(f, line, '\n');
		bool process_success = processLine(section, line);
		if (!process_success)
		{
			return false;
		}
	}

	//Print(std::cerr);

	return true;
}

bool Config::processLine(Config::iterator & section, std::string & linestr)
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
			else if (!SUPPRESS_ERROR)
			{
				//std::cout << "a line started with an equal sign or ended with an equal sign:" << std::endl
				//          << linestr << std::endl;
				return false;
			}
		}
		else if (linestr.find("include ") == 0)
		{
			//configfile include
			std::string dir;
			std::string relpath = linestr.erase(0, 8);
			std::string::size_type pos = filename.rfind('/');
			if (pos != std::string::npos) dir = filename.substr(0, pos);
			std::string path = GetAbsolutePath(dir, relpath);
			bool include_load_success = load(path);
			if (!SUPPRESS_ERROR && !include_load_success)
			{
				//std::cout << "the included file failed to load, bail" << std::endl;
				return false;
			}
		}
		else
		{
			//section header
			std::string section_name;
			section_name = Strip(linestr, '[');
			section_name = Strip(section_name, ']');
			section_name = Trim(section_name);

			// subsection
			size_t n = section_name.rfind('.');
			if (n != std::string::npos)
			{
				std::string parent = section_name.substr(0, n);
				std::string child = section_name.substr(n+1);

				/*
				SECTIONMAP::const_iterator parent_iter = sections.find(parent);
				if (!SUPPRESS_ERROR && (parent_iter == sections.end()))
				{
					std::cout << "warning: parent section " << parent << " doesn't exist. adding an empty one." << std::endl;
					return false;
				}

				Section::const_iterator child_iter = parent_iter->second.find(child);
				if (!SUPPRESS_ERROR && (child_iter != parent_iter->second.end()))
				{
					std::cout << "child already exists, this must be a duplicate section. error" << std::endl;
					return false;
				}
				*/
				sections[parent][child] = section_name;
			}
			/*
			SECTIONMAP::const_iterator already_exists = sections.find(section_name);
			if (!SUPPRESS_ERROR && (already_exists != sections.end()))
			{
				std::cout << "section " << section_name << " already exists, duplicate section in the file, error" << std::endl;
				return false;
				/// this shouldn't be an error case because included files will import sections.
				/// find a way to mark which sections were included, or keep a list of sections imported during this load?
				/// or perhaps just don't worry about it?
			}
			*/
			section = sections.insert(std::pair<std::string, Section>(section_name, Section())).first;
		}
	}

	return true;
}

void Config::print(std::ostream & out, bool with_brackets) const
{
	for (const_iterator im = sections.begin(); im != sections.end(); ++im)
	{
		if (!im->first.empty())
		{
			std::string sectionname = im->first;
			if (with_brackets) out << "[" << sectionname << "]" << std::endl;
			else out << sectionname << std::endl;
		}
		for (Section::const_iterator is = im->second.begin(); is != im->second.end(); ++is)
		{
			out << is->first << " = " << is->second << std::endl;
		}
		out << std::endl;
	}
}

bool Config::write() const
{
	return write(filename);
}

bool Config::write(std::string save_as) const
{
	std::ofstream f(save_as.c_str());
	if (!f) return false;

	print(f);
	return true;
}

QT_TEST(configfile_test)
{
	std::istringstream instream(
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
		"this is a duplicate = 2\n"
		"unterminated line = good?");

	Config testconfig;
	testconfig.load(instream);
	std::string tstr = "notfound";
	QT_CHECK(testconfig.get("", "variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(testconfig.get("section    dos??", "why won't you", tstr));
	QT_CHECK_EQUAL(tstr, "breeeak");
	tstr = "notfound";
	QT_CHECK(testconfig.get("", "variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(!testconfig.get("nosection", "novariable", tstr));
	QT_CHECK_EQUAL(tstr, "notfound");
	tstr = "notfound";
	std::vector<float> vec(3);
	QT_CHECK(testconfig.get("what about", "even vectors", vec));
	QT_CHECK_EQUAL(vec[0], 2.1f);
	QT_CHECK_EQUAL(vec[1], 0.9f);
	QT_CHECK_EQUAL(vec[2], 0.f);
	tstr = "notfound";
	QT_CHECK(testconfig.get("what about", "unterminated line", tstr));
	QT_CHECK_EQUAL(tstr, "good?");
	//testconfig.Print(std::cout);

	{
		Config::const_iterator i = testconfig.begin();
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
		Config::const_iterator i;
		testconfig.get("test section numero UNO", i);
		QT_CHECK_EQUAL(testconfig.get(i, "i'm so great", value), true);
		QT_CHECK_EQUAL(testconfig.get(i, "look at me", value), true);
	}
}

#include "pathmanager.h"
QT_TEST(config_include)
{
	std::stringbuf log;
	std::ostream info(&log), error(&log);
	PathManager path;
	path.Init(info, error);

	std::string bad_include_file = path.GetDataPath() + "/test/badinclude.cfg";
	std::string test_file = path.GetDataPath() + "/test/test.cfg";
	std::string verify_file = path.GetDataPath() + "/test/verify.cfg";

	bool cfg_bad_include_file_loaded = false;
	Config cfg_bad_include;
	cfg_bad_include_file_loaded = cfg_bad_include.load(bad_include_file);
	//cfg_bad_include.Print(std::cerr);
	QT_CHECK(!cfg_bad_include_file_loaded);

	bool cfg_test_file_loaded = false;
	Config cfg_test;
	cfg_test_file_loaded = cfg_test.load(test_file);
	//cfg_test.Print(std::cerr);
	QT_CHECK(cfg_test_file_loaded);

	bool cfg_verify_file_loaded = false;
	Config cfg_verify;
	cfg_verify_file_loaded = cfg_verify.load(verify_file);
	//cfg_verify.Print(std::cerr);

	QT_CHECK(cfg_verify_file_loaded);
	if (!(cfg_test_file_loaded && cfg_verify_file_loaded)) return;

	for (Config::const_iterator s = cfg_verify.begin(); s != cfg_verify.end(); ++s)
	{
		Config::const_iterator ts;
		QT_CHECK(cfg_test.get(s->first, ts, error));
		if (ts == cfg_test.end()) continue;

		for (Config::Section::const_iterator p = s->second.begin(); p != s->second.end(); ++p)
		{
			std::string value;
			QT_CHECK(cfg_test.get(ts, p->first, value, error));
			QT_CHECK(p->second == value);
		}
	}
}

