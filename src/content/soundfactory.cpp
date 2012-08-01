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

#include "soundfactory.h"
#include "sound/soundbuffer.h"
#include <fstream>

Factory<SOUNDBUFFER>::Factory() :
	m_default(new SOUNDBUFFER()),
	m_info(0, 0, 0, 0)
{
	// ctor
}

void Factory<SOUNDBUFFER>::init(const SOUNDINFO& value)
{
	m_info = value;
}

template <>
bool Factory<SOUNDBUFFER>::create(
	std::tr1::shared_ptr<SOUNDBUFFER> & sptr,
	std::ostream & error,
	const std::string & basepath,
	const std::string & path,
	const std::string & name,
	const empty&)
{
	const std::string abspath = basepath + "/" + path + "/" + name;
	std::string filepath = abspath + ".ogg";
	if (!std::ifstream(filepath.c_str()))
	{
		filepath = abspath + ".wav";
	}
	if (std::ifstream(filepath.c_str()))
	{
		std::tr1::shared_ptr<SOUNDBUFFER> temp(new SOUNDBUFFER());
		if (temp->Load(filepath, m_info, error))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

std::tr1::shared_ptr<SOUNDBUFFER> Factory<SOUNDBUFFER>::getDefault() const
{
	return m_default;
}
