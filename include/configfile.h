#ifndef _CONFIGFILE_H
#define _CONFIGFILE_H

#include <string>
#include <map>
#include <list>
#include <iostream>

#include "bucketed_hashmap.h"
#include "configvariable.h"

//see the user's guide at the bottom of the file
class CONFIGFILE
{
public:
	CONFIGFILE();
	CONFIGFILE(std::string fname);
	~CONFIGFILE();
	
	bool Load(std::string fname);
	bool Load(std::istream & f);
	void Clear();
	std::string LoadedFile() const {return filename;}
	
	//returns true if the param was found
	bool ClearParam(std::string param);
	
	//returns true if the param was found
	bool GetParam(std::string param, std::string & outvar) const;
	bool GetParam(std::string param, int & outvar) const;
	bool GetParam(std::string param, float & outvar) const;
	bool GetParam(std::string param, float * outvar) const; //for float[3]
	bool GetParam(std::string param, bool & outvar) const;
	
	///a higher level helper function to read from a configfile or print out an error
	template <typename T>
	bool GetParam(const std::string & param, T & output, std::ostream & error_output) const
	{
		if (!GetParam(param, output))
		{
			error_output << "Couldn't get parameter \"" << param << "\" from file \"" << LoadedFile() << "\"" << std::endl;
			return false;
		}
		return true;
	}
	
	// read points from configfile section
	void GetPoints(const std::string & sectionname, const std::string & paramprefix, std::vector <std::pair <double, double> > & output_points) const;
	
	///a higher level helper function to read to/write from a configfile based on the passed directional boolean.
	///if set_direction is true, then the param will be set to value.
	///if set_direction is false, then value will be set to the param.
	template <typename T>
	bool GetSetParam(const std::string & param, T & value, bool set_direction)
	{
		if (set_direction)
			return SetParam(param, value);
		else
			return GetParam(param, value);
	}

	//always returns true at the moment
	bool SetParam(std::string param, std::string invar);
	bool SetParam(std::string param, int invar);
	bool SetParam(std::string param, float invar);
	bool SetParam(std::string param, float * invar);
	bool SetParam(std::string param, bool invar);
	
	void GetSectionList(std::list <std::string> & sectionlistoutput) const;
	void GetParamList(std::list <std::string> & paramlistoutput) const {GetParamList(paramlistoutput, "");}
	void GetParamList(std::list <std::string> & paramlistoutput, std::string section) const; ///< returns param names only, not their sections
	
	void ChangeSectionName(std::string oldname, std::string newname);

	void DebugPrint(std::ostream & out);

	bool Write();
	bool Write(bool with_brackets);
	bool Write(bool with_brackets, std::string save_as);
	
	void SuppressError(bool newse) {SUPPRESS_ERROR = newse;}
	
	unsigned int GetNumParams() {return variables.GetTotalObjects();}

private:
	std::string filename;
	bucketed_hashmap <std::string, CONFIGVARIABLE> variables;
	bool SUPPRESS_ERROR;
	
	void Add(std::string & paramname, CONFIGVARIABLE & newvar);
	
	void ProcessLine(std::string & cursection, std::string linestr);
	
	std::string Trim(std::string instr);
	
	std::string Strip(std::string instr, char stripchar);
	
	std::string LCase(std::string instr);
};

#endif /* _CONFIGFILE_H */

/* USER GUIDE

Paste the file included below somewhere, then run this code:

	CONFIGFILE testconfig("/home/joe/.vdrift/test.cfg");
	testconfig.DebugPrint();
	string tstr = "notfound";
	cout << "!!! test vectors: " << endl;
	testconfig.GetParam("variable outside of", tstr);
	cout << tstr << endl;
	tstr = "notfound";
	testconfig.GetParam(".variable outside of", tstr);
	cout << tstr << endl;
	float vec[3];
	testconfig.GetParam("what about.even vectors", vec);
	cout << vec[0] << "," << vec[1] << "," << vec[2] << endl;

your output should be the debug print of all variables, then:

	!!! test vectors: 
	a section
	a section
	2.1,0.9,0


*/

/* EXAMPLE (not a good example -- tons of errors -- to demonstrate robustness)

#comment on the FIRST LINE??

		variable outside of=a section  

test section numero UNO
look at me = 23.4

i'm so great=   BANANA
#break!
[ section    dos??]
 why won't you = breeeak #trying to break it

what about ] # this malformed thing???
nope works = fine.
even vectors = 2.1,0.9,GAMMA
this is a duplicate = 0
this is a duplicate = 1
random = intermediary
this is a duplicate = 2


*/
