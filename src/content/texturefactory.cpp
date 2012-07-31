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

#include "content/texturefactory.h"
#include "texture.h"
#include <fstream>
#include <sstream>

Factory<TEXTURE>::Factory() :
	m_default(new TEXTURE()),
	m_size(TEXTUREINFO::LARGE),
	m_srgb(false)
{
	// ctor
}

void Factory<TEXTURE>::init(int max_size, bool use_srgb)
{
	m_size = max_size;
	m_srgb = use_srgb;

	// init default texture
	std::stringstream error;
	char white[] = {255, 255, 255, 255};
	TEXTUREINFO info;
	info.data = white;
	info.width = 1;
	info.height = 1;
	info.bytespp = 4;
	info.maxsize = TEXTUREINFO::Size(m_size);
	info.mipmap = false;
	info.srgb = m_srgb;
	m_default->Load("", info, error);
}

template <>
bool Factory<TEXTURE>::create(
	std::tr1::shared_ptr<TEXTURE> & sptr,
	std::ostream & error,
	const std::string & basepath,
	const std::string & path,
	const std::string & name,
	const TEXTUREINFO& info)
{
	const std::string abspath = basepath + "/" + path + "/" + name;
	if (info.data || std::ifstream(abspath.c_str()))
	{
		TEXTUREINFO info_temp = info;
		info_temp.srgb = m_srgb;
		info_temp.maxsize = TEXTUREINFO::Size(m_size);
		std::tr1::shared_ptr<TEXTURE> temp(new TEXTURE());
		if (temp->Load(abspath, info_temp, error))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

std::tr1::shared_ptr<TEXTURE> Factory<TEXTURE>::getDefault() const
{
	return m_default;
}
