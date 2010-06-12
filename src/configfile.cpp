#include "configfile.h"
#include "containeralgorithm.h"
#include "unittest.h"

#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>

CONFIGFILE::CONFIGFILE()
{
	filename = "";
	SUPPRESS_ERROR = false;
}

CONFIGFILE::CONFIGFILE(std::string fname)
{

	SUPPRESS_ERROR = false;
	Load(fname);
}

CONFIGFILE::~CONFIGFILE()
{
	Clear();
}

bool CONFIGFILE::GetParam(std::string param, int & outvar) const
{
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		if (param.substr(0, ppos).empty())
		{
			ppos++;
			param = param.substr(ppos, param.length() - ppos);
		}
	}
	
	const CONFIGVARIABLE * v = variables.Get(param);
	
	if (!v)
		return false;
	
	outvar = v->val_i;
	
	return true;
}

bool CONFIGFILE::GetParam(std::string param, bool & outvar) const
{
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		if (param.substr(0, ppos).empty())
		{
			ppos++;
			param = param.substr(ppos, param.length() - ppos);
		}
	}
	
	const CONFIGVARIABLE * v = variables.Get(param);
	
	if (!v)
		return false;
	
	outvar = v->val_b;
	
	return true;
}

bool CONFIGFILE::GetParam(std::string param, float & outvar) const
{
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		if (param.substr(0, ppos).empty())
		{
			ppos++;
			param = param.substr(ppos, param.length() - ppos);
		}
	}
	
	const CONFIGVARIABLE * v = variables.Get(param);
	
	if (!v)
		return false;
	
	outvar = v->val_f;
	
	return true;
}

bool CONFIGFILE::GetParam(std::string param, float * outvar) const
{
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		if (param.substr(0, ppos).empty())
		{
			ppos++;
			param = param.substr(ppos, param.length() - ppos);
		}
	}
	
	const CONFIGVARIABLE * v = variables.Get(param);
	
	if (!v)
		return false;
	
	for (int i = 0; i < 3; i++)
		outvar[i] = v->val_v[i];
	
	return true;
}

bool CONFIGFILE::GetParam(std::string param, std::string & outvar) const
{
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		if (param.substr(0, ppos).empty())
		{
			ppos++;
			param = param.substr(ppos, param.length() - ppos);
		}
	}
	
	const CONFIGVARIABLE * v = variables.Get(param);
	
	if (!v)
		return false;
	
	outvar = v->val_s;
	
	return true;
}

void CONFIGFILE::GetPoints(const std::string & sectionname, const std::string & paramprefix, std::vector <std::pair <double, double> > & output_points) const
{
	std::list <std::string> params;
	GetParamList(params, sectionname);
	for (std::list <std::string>::iterator i = params.begin(); i != params.end(); i++)
	{
		if (i->find(paramprefix) == 0)
		{
			float point[3] = {0, 0, 0};
			if (GetParam(sectionname+"."+*i, point))
			{
				output_points.push_back(std::make_pair(point[0], point[1]));
			}
		}
	}
}

void CONFIGFILE::Clear()
{
	filename.clear();
	variables.Clear();
}

void CONFIGFILE::Add(std::string & paramname, CONFIGVARIABLE & newvar)
{
	variables.Set(paramname, newvar);
}

bool CONFIGFILE::Load(std::string fname)
{
	filename = fname;
	
	//work std::string
	std::string ws;
	
	std::ifstream f;
	f.open(fname.c_str());
	
	if (!f && !SUPPRESS_ERROR)
	{
		return false;
	}
	
	return Load(f);
}

bool CONFIGFILE::Load(std::istream & f)
{
	std::string cursection = "";
	const int MAXIMUMCHAR = 1024;
	char trashchar[MAXIMUMCHAR];
	
	while (f && !f.eof())
	{
		f.getline(trashchar, MAXIMUMCHAR, '\n');
		ProcessLine(cursection, trashchar);
	}
	
	return true;
}

std::string CONFIGFILE::Trim(std::string instr)
{
	CONFIGVARIABLE trimmer;
	std::string outstr = trimmer.strTrim(instr);
	return outstr;
}

void CONFIGFILE::ProcessLine(std::string & cursection, std::string linestr)
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
				CONFIGVARIABLE newvar;
				newvar.section = cursection;
				newvar.name = name;
				newvar.Set(val);
				
				std::string paramname = name;
				if (!cursection.empty())
					paramname = cursection + "." + paramname;
				
				Add(paramname, newvar);
			}
		}
		else
		{
			//section header
			linestr = Strip(linestr, '[');
			linestr = Strip(linestr, ']');
			linestr = Trim(linestr);
			cursection = linestr;
		}
	}
}

std::string CONFIGFILE::Strip(std::string instr, char stripchar)
{
	std::string::size_type pos = 0;
	std::string outstr = "";
	
	while (pos < /*(int)*/ instr.length())
	{
		if (instr.c_str()[pos] != stripchar)
			outstr = outstr + instr.substr(pos, 1);
		
		pos++;
	}
	
	return outstr;
}

void CONFIGFILE::DebugPrint(std::ostream & out)
{
	out << "*** " << filename << " ***" << std::endl << std::endl;
	
	std::list <CONFIGVARIABLE> vlist;
	for (bucketed_hashmap <std::string, CONFIGVARIABLE>::iterator i = variables.begin(); i != variables.end(); ++i)
	{
		vlist.push_back(*i);
	}
	vlist.sort();
	
	for (std::list <CONFIGVARIABLE>::iterator i = vlist.begin(); i != vlist.end(); ++i)
	{
		i->DebugPrint(out);
	}
}

std::string CONFIGFILE::LCase(std::string instr)
{
	CONFIGVARIABLE lcaser;
	std::string outstr = lcaser.strLCase(instr);
	return outstr;
}

bool CONFIGFILE::SetParam(std::string param, int invar)
{
	char tc[256];
	
	sprintf(tc, "%i", invar);
	
	std::string tstr = tc;
	
	return SetParam(param, tstr);
}

bool CONFIGFILE::SetParam(std::string param, bool invar)
{
	//char tc[256];
	
	//sprintf(tc, "%i", invar);
	
	std::string tstr = "false";
	
	if (invar)
		tstr = "true";
	
	return SetParam(param, tstr);
}

bool CONFIGFILE::SetParam(std::string param, float invar)
{
	char tc[256];
	
	sprintf(tc, "%f", invar);
	
	std::string tstr = tc;
	
	return SetParam(param, tstr);
}

bool CONFIGFILE::SetParam(std::string param, float * invar)
{
	char tc[256];
	
	sprintf(tc, "%f,%f,%f", invar[0], invar[1], invar[2]);
	
	std::string tstr = tc;
	
	return SetParam(param, tstr);
}

bool CONFIGFILE::SetParam(std::string param, std::string invar)
{
	CONFIGVARIABLE newvar;
	
	newvar.name = param;
	newvar.section = "";
	std::string::size_type ppos;
	ppos = param.find(".", 0);
	if (ppos < param.length())
	{
		newvar.section = param.substr(0, ppos);
		ppos++;
		newvar.name = param.substr(ppos, param.length() - ppos);
	}
	
	newvar.Set(invar);
	
	Add(param, newvar);
	
	return true;
}

bool CONFIGFILE::Write(bool with_brackets)
{
	return Write(with_brackets, filename);
}

bool CONFIGFILE::Write(bool with_brackets, std::string save_as)
{
	std::ofstream f;
	f.open(save_as.c_str());
	
	if (f)
	{
		std::list <CONFIGVARIABLE> vlist;
	
		for (bucketed_hashmap <std::string, CONFIGVARIABLE>::iterator i = variables.begin(); i != variables.end(); ++i)
		{
			vlist.push_back(*i);
		}
		
		vlist.sort();
		
		std::string cursection = "";
		for (std::list <CONFIGVARIABLE>::iterator cur = vlist.begin(); cur != vlist.end(); ++cur)
		{
			if (cur->section == "")
			{
				f << cur->name << " = " << cur->val_s << std::endl;
			}
			else
			{
				if (cur->section != cursection)
				{
					f << std::endl;
					cursection = cur->section;
					
					if (with_brackets)
						f << "[ " << cur->section << " ]" << std::endl;
					else
						f << cur->section << std::endl;
				}
				
				f << cur->name << " = " << cur->val_s << std::endl;
			}
		}
		
		f.close();
		return true;
	}
	else
		return false;
}

bool CONFIGFILE::Write()
{
	return Write(true);
}

bool CONFIGFILE::ClearParam(std::string param)
{
	return variables.Erase(param);
}

void CONFIGFILE::GetSectionList(std::list <std::string> & sectionlistoutput) const
{
	sectionlistoutput.clear();
	std::map <std::string, bool> templist;
	for (bucketed_hashmap <std::string, CONFIGVARIABLE>::const_iterator i = variables.begin(); i != variables.end(); ++i)
	{
		templist[i->section] = true;
	}
	
	for (std::map <std::string, bool>::iterator i = templist.begin(); i != templist.end(); ++i)
	{
		sectionlistoutput.push_back(i->first);
	}
}

void CONFIGFILE::GetParamList(std::list <std::string> & paramlistoutput, std::string sel_section) const
{
	bool all = false;
	if (sel_section == "")
		all = true;
	
	//cout << "++++++++++" << variables.GetTotalObjects() << std::endl;
	
	paramlistoutput.clear();
	std::map <std::string, bool> templist;
	for (bucketed_hashmap <std::string, CONFIGVARIABLE>::const_iterator i = variables.begin(); i != variables.end(); ++i)
	{
		if (all)
			templist[i->section+"."+i->name] = true;
		else if (i->section == sel_section)
		{
			templist[i->name] = true;
		}
	}
	
	for (std::map <std::string, bool>::iterator i = templist.begin(); i != templist.end(); ++i)
	{
		paramlistoutput.push_back(i->first);
	}
}

void CONFIGFILE::ChangeSectionName(std::string oldname, std::string newname)
{
	std::list <std::string> paramlist;
	GetParamList(paramlist, oldname);
	
	for (std::list <std::string>::iterator i = paramlist.begin(); i != paramlist.end(); ++i)
	{
		std::string value;
		bool success = GetParam(oldname+"."+*i, value);
		assert(success);
		SetParam(newname+"."+*i, value);
		success = ClearParam(oldname+"."+*i);
		assert(success);
	}
}

QT_TEST(configfile_test)
{
	std::stringstream instream;
	instream << "\n#comment on the FIRST LINE??\n\n"
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
	
	CONFIGFILE testconfig;
	testconfig.Load(instream);
	std::string tstr = "notfound";
	QT_CHECK(testconfig.GetParam("variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam(".variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam("section    dos??.why won't you", tstr));
	QT_CHECK_EQUAL(tstr, "breeeak");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam("variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(testconfig.GetParam(".variable outside of", tstr));
	QT_CHECK_EQUAL(tstr, "a section");
	tstr = "notfound";
	QT_CHECK(!testconfig.GetParam("nosection.novariable", tstr));
	QT_CHECK_EQUAL(tstr, "notfound");
	tstr = "notfound";
	float vec[3];
	QT_CHECK(testconfig.GetParam("what about.even vectors", vec));
	QT_CHECK_EQUAL(vec[0], 2.1f);
	QT_CHECK_EQUAL(vec[1], 0.9f);
	QT_CHECK_EQUAL(vec[2], 0.f);
	//testconfig.DebugPrint(std::cout);
	
	{
		std::list <std::string> slist;
		testconfig.GetSectionList(slist);
		slist.sort();
		std::list <std::string>::iterator i = slist.begin();
		QT_CHECK_EQUAL(*i, "");
		i++;
		QT_CHECK_EQUAL(*i, "section    dos??");
		i++;
		QT_CHECK_EQUAL(*i, "test section numero UNO");
		i++;
		QT_CHECK_EQUAL(*i, "what about");
		i++;
		QT_CHECK(i == slist.end());
	}
	
	{
		std::list <std::string> slist;
		testconfig.GetParamList(slist, "test section numero UNO");
		slist.sort();
		std::list <std::string>::iterator i = slist.begin();
		QT_CHECK_EQUAL(*i, "i'm so great");
		i++;
		QT_CHECK_EQUAL(*i, "look at me");
		i++;
		QT_CHECK(i == slist.end());
	}
}

