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
	void Set(const std::string & lang_id, std::ostream & info, std::ostream & error);

	/// translation operator
	std::string operator()(const std::string & str) const;

	/// get code page string from language id
	std::string GetCodePage() const;

private:
	std::string m_lang_id;

	void Init(std::ostream & info, std::ostream & error);
};

#endif // _GUILANGUAGE_H

