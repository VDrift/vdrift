#ifndef _CONFIGVARIABLE_H
#define _CONFIGVARIABLE_H

#include <string>

class CONFIGVARIABLE
{
public:
	CONFIGVARIABLE();
	
	CONFIGVARIABLE(const CONFIGVARIABLE & other);
	
	CONFIGVARIABLE & operator=(const CONFIGVARIABLE & other);
	
	bool operator<(const CONFIGVARIABLE & other);
	
	CONFIGVARIABLE & CopyFrom(const CONFIGVARIABLE & other);
	
	const std::string GetFullName() const;
	
	void Set(std::string newval);

	void DebugPrint(std::ostream & out);

	std::string strLTrim(std::string instr);
	
	std::string strRTrim(std::string instr);
	
	std::string strTrim(std::string instr);
	
	std::string strLCase(std::string instr);
	
	std::string section;
	std::string name;
	std::string val_s;
	int val_i;
	float val_f;
	float val_v[3];
	bool val_b;
	bool written;
	CONFIGVARIABLE * next;
};

#endif // _CONFIGVARIABLE_H
