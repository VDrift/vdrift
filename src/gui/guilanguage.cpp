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
#include <ostream>
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

void GuiLanguage::Set(const std::string & lang_id, std::ostream & info, std::ostream & error)
{
	if (m_lang_id != lang_id)
	{
		m_lang_id = lang_id;
		Init(info, error);
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

void GuiLanguage::Init(std::ostream & info, std::ostream & error)
{
	const char * sys_locale = setlocale(LC_ALL, "");
	info << "Default locale LC_ALL = " << sys_locale << std::endl;

	#ifndef _WIN32
	const char * app_locale = setlocale(LC_MESSAGES, m_lang_id.c_str());
	if (app_locale == NULL)
	{
		error << "Failed to set LC_MESSAGES = " << m_lang_id << std::endl;

		setenv("LANGUAGE", m_lang_id.c_str(), 1);
		info << "Set LANGUAGE = " << m_lang_id << std::endl;
	}
	else
	{
		info << "Set LC_MESSAGES = " << app_locale << std::endl;
	}
	#else
	// force locale on windows
	_putenv_s("LANGUAGE", m_lang_id.c_str());
	info << "Set LANGUAGE = " << m_lang_id << std::endl;
	#endif

	textdomain("vdrift");

	const char * locale_dir = bindtextdomain("vdrift", LOCALE_DIR);
	if (locale_dir == NULL)
		error << "Failed to set locale base directory to: " << LOCALE_DIR << std::endl;
	else
		info << "Set locale base directory to: " << locale_dir << std::endl;

	std::string cp = "CP" + GetCodePage();
	const char * codeset = bind_textdomain_codeset("vdrift", cp.c_str());
	if (codeset == NULL)
		error << "Failed to set output codeset to: " << cp << std::endl;
	else
		info << "Set output codeset to: " << codeset << std::endl;
}
