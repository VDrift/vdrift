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

#ifndef _GUILANGUAGE_H
#define _GUILANGUAGE_H

#include <ostream>
#include <string>
#include <map>

class GUILANGUAGE
{
public:
	GUILANGUAGE();

	~GUILANGUAGE();

	/// language path to the directory containg language files(UTF-8)
	/// en.txt, de.txt, fr.txt ...
	void Init(const std::string & lang_path);

	/// langage id is a two char string
	/// en, de, fr, ru ...
	void Set(const std::string & lang_id, std::ostream & error);

	/// translation operator
	const std::string & operator()(const std::string & str) const;

	/// get code page string from language id
	static const std::string & GetCodePageId(const std::string & lang_id);

private:
	std::map<std::string, std::string> m_strings;
	std::string m_lang_path;
	std::string m_lang_id;

	// conversion descriptor
	void * m_iconv;

	/// load current language string map
	void LoadLanguage(std::ostream & error);
};

// implementation

inline const std::string & GUILANGUAGE::operator()(const std::string & str) const
{
	std::map<std::string, std::string>::const_iterator i = m_strings.find(str);
	if (i != m_strings.end())
		return i->second;
	return str;
}

#endif // _GUILANGUAGE_H

