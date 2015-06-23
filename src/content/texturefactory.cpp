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

#include "texturefactory.h"
#include "graphics/texture.h"
#include <fstream>
#include <sstream>

Factory<Texture>::Factory() :
	m_default(new Texture()),
	m_zero(new Texture()),
	m_size(TextureInfo::LARGE),
	m_compress(true),
	m_srgb(false)
{
	// ctor
}

void Factory<Texture>::init(int max_size, bool use_srgb, bool compress)
{
	m_size = max_size;
	m_srgb = use_srgb;
	m_compress = compress;

	// init default texture
	std::ostringstream error;
	unsigned char one[] = {255u, 255u, 255u, 255u};
	unsigned char zero[] = {0u, 0u, 0u, 0u};

	TextureInfo info;
	info.width = 1;
	info.height = 1;
	info.bytespp = 4;
	info.maxsize = TextureInfo::Size(m_size);
	info.mipmap = false;
	info.srgb = m_srgb;

	info.data = one;
	m_default->Load("", info, error);

	info.data = zero;
	m_zero->Load("", info, error);
}

template <>
bool Factory<Texture>::create(
	std::shared_ptr<Texture> & sptr,
	std::ostream & error,
	const std::string & basepath,
	const std::string & path,
	const std::string & name,
	const TextureInfo& info)
{
	const std::string abspath = basepath + "/" + path + "/" + name;
	if (info.data || std::ifstream(abspath.c_str()))
	{
		TextureInfo info_temp = info;
		info_temp.srgb = info.compress && m_srgb; 			// non compressible means non color data
		info_temp.compress = info.compress && m_compress;	// allow to disable compression
		info_temp.maxsize = TextureInfo::Size(m_size);
		std::shared_ptr<Texture> temp(new Texture());
		if (temp->Load(abspath, info_temp, error))
		{
			sptr = temp;
			return true;
		}
	}
	return false;
}

const std::shared_ptr<Texture> & Factory<Texture>::getDefault() const
{
	return m_default;
}

const std::shared_ptr<Texture> & Factory<Texture>::getZero() const
{
	return m_zero;
}
