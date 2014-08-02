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

#include "guilanguage.h"
#include "definitions.h"
#include <libintl.h>
#include <cstdlib>

GuiLanguage::GuiLanguage() :
	m_lang_id("en")
{
	// ctor
}

GuiLanguage::~GuiLanguage()
{
	// dtor
}

void GuiLanguage::Set(const std::string & lang_id, std::ostream & error)
{
	if (m_lang_id != lang_id)
	{
		m_lang_id = lang_id;
		Init(error);
	}
}

std::string GuiLanguage::operator()(const std::string & str) const
{
	return gettext(str.c_str());
}

std::string GuiLanguage::GetCodePage() const
{
	std::string cp = gettext("_CODEPAGE_");
	if (cp == "_CODEPAGE_")
		cp = "1252";
	return cp;
}

void GuiLanguage::Init(std::ostream & error)
{
	// set system locale
	setlocale(LC_ALL, "");

	#ifndef _WIN32
	std::string app_locale = setlocale(LC_MESSAGES, m_lang_id.c_str());
	if (app_locale.empty())
	{
		error << "Set LANGUAGE to " << m_lang_id << std::endl;
		setenv("LANGUAGE", m_lang_id.c_str(), 1);
	}
	#else
	// force locale on windows
	_putenv_s("LANGUAGE", m_lang_id.c_str());
	#endif

	textdomain("vdrift");
	bindtextdomain("vdrift", LOCALE_DIR);

	std::string cp = "CP" + GetCodePage();
	bind_textdomain_codeset("vdrift", cp.c_str());
}
