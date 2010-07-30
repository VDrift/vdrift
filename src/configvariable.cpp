#include "configvariable.h"

#include <iostream>
#include <cstdlib> //atoi, atof

CONFIGVARIABLE::CONFIGVARIABLE()
{
	val_s = "";
	val_i = 0;
	val_f = 0;
	val_b = false;
	int i;
	for (i = 0; i < 3; i++)
		val_v[i] = 0;
	
	next = NULL;
}

CONFIGVARIABLE::CONFIGVARIABLE(const CONFIGVARIABLE & other)
{
	CopyFrom(other);
}

CONFIGVARIABLE & CONFIGVARIABLE::operator=(const CONFIGVARIABLE & other)
{
	return CopyFrom(other);
}

bool CONFIGVARIABLE::operator<(const CONFIGVARIABLE & other)
{
	return (section + "." + name < other.section + "." + other.name);
}

CONFIGVARIABLE & CONFIGVARIABLE::CopyFrom(const CONFIGVARIABLE & other)
{
	section = other.section;
	name = other.name;
	val_s = other.val_s;
	val_i = other.val_i;
	val_f = other.val_f;
	val_b = other.val_b;
	
	for (int i = 0; i < 3; i++)
		val_v[i] = other.val_v[i];
	
	return *this;
}

const std::string CONFIGVARIABLE::GetFullName() const
{
	std::string outstr = "";
	
	if (section != "")
		outstr = outstr + section + ".";
	outstr = outstr + name;
	
	return outstr;
}

void CONFIGVARIABLE::Set(std::string newval)
{
	newval = strTrim(newval);
	
	val_i = std::atoi(newval.c_str());
	val_f = std::atof(newval.c_str());
	val_s = newval;
	
	val_b = false;
	if (val_i == 0)
		val_b = false;
	if (val_i == 1)
		val_b = true;
	if (strLCase(newval) == "true")
		val_b = true;
	if (strLCase(newval) == "false")
		val_b = false;
	if (strLCase(newval) == "on")
		val_b = true;
	if (strLCase(newval) == "off")
		val_b = false;
	
	//now process as vector information
	int pos = 0;
	int arraypos = 0;
	std::string::size_type nextpos = newval.find(",", pos);
	std::string frag;
	
	while (nextpos < newval.length() && arraypos < 3)
	{
		frag = newval.substr(pos, nextpos - pos);
		val_v[arraypos] = atof(frag.c_str());
		
		pos = nextpos+1;
		arraypos++;
		nextpos = newval.find(",", pos);
	}
	
	//don't forget the very last one
	if (arraypos < 3)
	{
		frag = newval.substr(pos, newval.length() - pos);
		val_v[arraypos] = atof(frag.c_str());
	}
}

void CONFIGVARIABLE::DebugPrint(std::ostream & out)
{
	if (section != "")
		out << section << ".";
	out << name << std::endl;
	out << "std::string: " << val_s << std::endl;
	out << "int: " << val_i << std::endl;
	out << "float: " << val_f << std::endl;
	out << "vector: (" << val_v[0] << "," << val_v[1] << "," << val_v[2] << ")" << std::endl;
	out << "bool: " << val_b << std::endl;
	
	out << std::endl;
}

std::string CONFIGVARIABLE::strLTrim(std::string instr)
{
	return instr.erase(0, instr.find_first_not_of(" \t"));
}

std::string CONFIGVARIABLE::strRTrim(std::string instr)
{
	if (instr.find_last_not_of(" \t") != std::string::npos)
		return instr.erase(instr.find_last_not_of(" \t") + 1);
	else
		return instr;
}

std::string CONFIGVARIABLE::strTrim(std::string instr)
{
	return strLTrim(strRTrim(instr));
}

std::string CONFIGVARIABLE::strLCase(std::string instr)
{
	char tc[2];
	tc[1] = '\0';
	std::string outstr = "";
	
	std::string::size_type pos = 0;
	while (pos < instr.length())
	{
		if (instr.c_str()[pos] <= 90 && instr.c_str()[pos] >= 65)
		{
			tc[0] = instr.c_str()[pos] + 32;
			std::string tstr = tc;
			outstr = outstr + tc;
		}
		else
			outstr = outstr + instr.substr(pos, 1);
		
		pos++;
	}
	
	return outstr;
}
