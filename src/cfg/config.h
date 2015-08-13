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

#ifndef _CONFIG_H
#define _CONFIG_H

#include <set>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>

/// container slice wrapper

template <typename Iter>
struct Slice
{
	Slice(Iter abegin, Iter aend) : begin(abegin), end(aend) { }
	Iter begin, end;
};

template <typename Iter>
inline std::istream & operator>>(std::istream & stream, Slice<Iter> & out)
{
	for (Iter i = out.begin; i != out.end && !stream.eof(); ++i)
	{
		std::string str;
		std::getline(stream, str, ',');
		std::istringstream s(str);
		s >> *i;
	}
	return stream;
}

/// see the user's guide at the bottom of the file
class Config
{
public:
	typedef std::map<std::string, std::string> Section;
	typedef std::map<std::string, Section> SectionMap;
	typedef SectionMap::const_iterator const_iterator;
	typedef SectionMap::iterator iterator;

	Config();

	Config(const std::string & fname);

	~Config();

	bool load(const std::string & fname);

	bool load(std::istream & f);

	void clear();

	void print(std::ostream & out, bool with_brackets = true) const;

	bool write() const;

	bool write(std::string save_as) const;

	const std::string & name() const;

	void suppressError(bool newse);

	const_iterator begin() const;

	const_iterator end() const;

	size_t size() const;

	/// will create section if not available
	void get(const std::string & section, iterator & it);

	bool get(const std::string & section, const_iterator & it) const;

	bool get(const std::string & section, const_iterator & it, std::ostream & error) const;

	template <typename T>
	bool get(const const_iterator & section, const std::string & param, T & output) const;

	template <typename T>
	bool get(const const_iterator & section, const std::string & param, std::vector<T> & out) const;

	template <typename T>
	bool get(const const_iterator & section, const std::string & param, T & output, std::ostream & error) const;

	template <typename T>
	bool get(const std::string & section, const std::string & param, T & output) const;

	template <typename T>
	bool get(const std::string & section, const std::string & param, T & output, std::ostream & error) const;

	/// will create param if not available
	template <typename T>
	void set(iterator section, const std::string & param, const T & invar);

	template <typename T>
	void set(iterator section, const std::string & param, const std::vector<T> & invar);

	template <typename T>
	void set(const std::string & section, const std::string & param, const T & invar);

private:
	SectionMap sections;
	std::set<std::string> include;
	std::string filename;
	bool SUPPRESS_ERROR;

	bool processLine(Config::iterator & section, std::string & linestr);
};

// implementation

inline const std::string & Config::name() const
{
	return filename;
}

inline void Config::suppressError(bool newse)
{
	SUPPRESS_ERROR = newse;
}

inline Config::const_iterator Config::begin() const
{
	return sections.begin();
}

inline Config::const_iterator Config::end() const
{
	return sections.end();
}

inline size_t Config::size() const
{
	return sections.size();
}

inline bool Config::get(const std::string & section, const_iterator & it) const
{
	it = sections.find(section);
	if (it != sections.end())
	{
		return true;
	}
	return false;
}

inline void Config::get(const std::string & section, iterator & it)
{
	it = sections.insert(std::pair<const std::string, Section>(section, Section())).first;
}

inline bool Config::get(const std::string & section, const_iterator & it, std::ostream & error) const
{
	if (!get(section, it))
	{
		error << "Couldn't get section \"" << section << "\" from \"" << filename << "\"" << std::endl;
		return false;
	}
	return true;
}

template <typename T>
inline bool Config::get(const const_iterator & section, const std::string & param, T & output) const
{
	assert(section != sections.end());
	Section::const_iterator i = section->second.find(param);
	if (i != section->second.end())
	{
		std::istringstream s(i->second);
		s >> std::boolalpha >> output;
		return true;
	}
	return false;
}

template <typename T>
inline bool Config::get(const const_iterator & section, const std::string & param, std::vector<T> & out) const
{
	assert(section != sections.end());
	Section::const_iterator i = section->second.find(param);
	if (i != section->second.end())
	{
		std::istringstream st(i->second);
		if (out.size() > 0)
		{
			// set vector
			for (size_t i = 0; i < out.size() && !st.eof(); ++i)
			{
				std::string str;
				std::getline(st, str, ',');
				std::istringstream s(str);
				s >> out[i];
			}
		}
		else
		{
			// fill vector
			while (!st.eof())
			{
				std::string str;
				std::getline(st, str, ',');
				std::istringstream s(str);
				T value;
				s >> value;
				out.push_back(value);
			}
		}
		return true;
	}
	return false;
}

template <typename T>
inline bool Config::get(const const_iterator & section, const std::string & param, T & output, std::ostream & error) const
{
	assert(section != sections.end());
	if (!get(section, param, output))
	{
		error << "Couldn't get parameter \"" << section->first << "." << param << "\" from \"" << filename << "\"" << std::endl;
		return false;
	}
	return true;
}

template <typename T>
inline bool Config::get(const std::string & section, const std::string & param, T & output) const
{
	const_iterator it = end();
	return get(section, it) && get(it, param, output);
}

template <typename T>
inline bool Config::get(const std::string & section, const std::string & param, T & output, std::ostream & error) const
{
	const_iterator it = end();
	return get(section, it, error) && get(it, param, output, error);
}

template <typename T>
inline void Config::set(iterator section, const std::string & param, const T & invar)
{
	if (section != sections.end())
	{
		std::ostringstream s;
		s << std::boolalpha << invar;
		section->second[param] = s.str();
	}
}

template <typename T>
inline void Config::set(iterator section, const std::string & param, const std::vector<T> & invar)
{
	if (section != sections.end())
	{
		std::ostringstream s;
		typename std::vector<T>::const_iterator it = invar.begin();
		while (it < invar.end() - 1)
		{
			s << *it++ << ",";
		}
		s << *it;
		section->second[param] = s.str();
	}
}

template <typename T>
inline void Config::set(const std::string & section, const std::string & param, const T & invar)
{
	iterator it;
	get(section, it);
	set(it, param, invar);
}

// specializations

template <>
inline bool Config::get(const const_iterator & section, const std::string & param, std::string & output) const
{
	assert(section != sections.end());
	Section::const_iterator i = section->second.find(param);
	if (i != section->second.end())
	{
		output = i->second;
		return true;
	}
	return false;
}

template <>
inline bool Config::get(const const_iterator & section, const std::string & param, bool & output) const
{
	assert(section != sections.end());
	Section::const_iterator i = section->second.find(param);
	if (i != section->second.end())
	{
		output = false;
		output = (i->second == "1" || i->second == "true" || i->second == "on");
		return true;
	}
	return false;
}

template <>
inline void Config::set(iterator section, const std::string & param, const std::string & invar)
{
	if (section != sections.end())
	{
		section->second[param] = invar;
	}
}

#endif /* _CONFIG_H */

/* USER GUIDE

Paste the file included below somewhere, then run this code:

	Config testconfig("/home/joe/.vdrift/test.cfg");
	testconfig.DebugPrint();
	string tstr = "notfound";
	cout << "!!! test vectors: " << endl;
	testconfig.get("variable outside of", tstr);
	cout << tstr << endl;
	tstr = "notfound";
	testconfig.get(".variable outside of", tstr);
	cout << tstr << endl;
	float vec[3];
	testconfig.get("what about.even vectors", vec);
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
