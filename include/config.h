#ifndef _CONFIG_H
#define _CONFIG_H

#include <set>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

template <class T>
void operator << (std::ostream & lhs, const std::vector<T> & rhs)
{
	typename std::vector<T>::const_iterator it = rhs.begin();
	while (it < rhs.end() - 1)
	{
		lhs << *it++ << ",";
	}
	lhs << *it;
}

template <class T>
void operator >> (std::istream & lhs, std::vector<T> & rhs)
{
	if (rhs.size() > 0)
	{
		// set vector
		for (size_t i = 0; i < rhs.size() && !lhs.eof(); ++i)
		{
			std::string str;
			std::getline(lhs, str, ',');
			std::stringstream s(str);
			s >> rhs[i];
		}
	}
	else
	{
		// fill vector
		while (!lhs.eof())
		{
			std::string str;
			std::getline(lhs, str, ',');
			std::stringstream s(str);
			T value;
			s >> value;
			rhs.push_back(value);
		}
	}
}

//see the user's guide at the bottom of the file
class CONFIG
{
public:
	CONFIG();
	
	CONFIG(std::string fname);
	
	~CONFIG();
	
	bool Load(std::string fname);
	
	bool Load(std::istream & f);
	
	void Clear();
	
	void DebugPrint(std::ostream & out, bool with_brackets = true) const;
	
	bool Write(bool with_brackets = true) const;
	
	bool Write(bool with_brackets, std::string save_as) const;
	
	void SuppressError(bool newse) {SUPPRESS_ERROR = newse;}
	
	const std::string & GetName() const {return filename;}
	
	typedef std::map<std::string, std::string> SECTION;
	
	typedef std::map<std::string, SECTION> SECTIONMAP;
	
	typedef SECTIONMAP::const_iterator const_iterator;
	
	typedef SECTIONMAP::iterator iterator;
	
	const_iterator begin() const {return sections.begin();}
	
	const_iterator end() const {return sections.end();}
	
	size_t size() const {return sections.size();}
		
	bool GetSection(const std::string & section, const_iterator & it) const
	{
		const_iterator i = sections.find(section);
		if (i != sections.end())
		{
			it = i;
			return true;
		}
		return false;
	}
	
	bool GetSection(const std::string & section, const_iterator & it, std::ostream & error_output) const
	{
		if (!GetSection(section, it))
		{
			error_output << "Couldn't get section \"" << section << "\" from \"" << filename << "\"" << std::endl;
			return false;
		}
		return true;
	}
	
	bool GetParam(const const_iterator & section, const std::string & param, std::string & output) const
	{
		SECTION::const_iterator i = section->second.find(param);
		if (i != section->second.end())
		{
			output = i->second;
			return true;
		}
		return false;
	}

	template <typename T>
	bool GetParam(const const_iterator & section, const std::string & param, T & output) const
	{
		SECTION::const_iterator i = section->second.find(param);
		if (i != section->second.end())
		{
			std::stringstream st(i->second);
			st >> std::boolalpha >> output;
			return true;
		}
		return false;
	}
		
	template <typename T>
	bool GetParam(const const_iterator & section, const std::string & param, T & output, std::ostream & error_output) const
	{
		if (!GetParam(section, param, output))
		{
			error_output << "Couldn't get parameter \"" << section->first << "." << param << "\" from \"" << filename << "\"" << std::endl;
			return false;
		}
		return true;
	}

	template <typename T>
	bool GetParam(const std::string & section, const std::string & param, T & output) const
	{
		const_iterator it = end();
		return GetSection(section, it) && GetParam(it, param, output);
	}
	
	template <typename T>
	bool GetParam(const std::string & section, const std::string & param, T & output, std::ostream & error_output) const
	{
		const_iterator it = end();
		return GetSection(section, it, error_output) && GetParam(it, param, output, error_output);
	}
	
	/// will create section if not available
	void GetSection(const std::string & section, iterator & it)
	{
		it = sections.insert(std::pair<const std::string, SECTION>(section, SECTION())).first;
	}
	
	/// will create param if not available
	void SetParam(iterator section, const std::string & param, const std::string & invar)
	{
		if (section != sections.end())
		{
			section->second[param] = invar;
		}
	}
	
	/// will create param if not available
	template <typename T>
	void SetParam(iterator section, const std::string & param, const T & invar)
	{
		if (section != sections.end())
		{
			std::stringstream st;
			st << std::boolalpha << invar;
			section->second[param] = st.str();
		}
	}

	template <typename T>
	void SetParam(const std::string & section, const std::string & param, const T & invar)
	{
		iterator it;
		GetSection(section, it);
		SetParam(it, param, invar);
	}
	
private:
	SECTIONMAP sections;
	std::set<std::string> include;
	std::string filename;
	bool SUPPRESS_ERROR;
	
	void ProcessLine(CONFIG::iterator & section, std::string & linestr);
};

#endif /* _CONFIG_H */

/* USER GUIDE

Paste the file included below somewhere, then run this code:

	CONFIG testconfig("/home/joe/.vdrift/test.cfg");
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

your output should be the debug print of all sections, then:

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
