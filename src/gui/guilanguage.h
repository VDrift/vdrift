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

#include <iosfwd>
#include <string>

class GuiLanguage
{
public:
	GuiLanguage();

	~GuiLanguage();

	/// langage id is a two char string
	/// en, de, fr, ru ...
	void Set(const std::string & lang_id, std::ostream & error);

	/// translation operator
	const std::string & operator()(const std::string & str) const;

	/// get code page string from language id
	static const std::string & GetCodePageId(const std::string & lang_id);

private:
	std::string m_lang_id;
	void * m_iconv;

	/// load current language string map
	void LoadLanguage(std::ostream & error);
};

#endif // _GUILANGUAGE_H

