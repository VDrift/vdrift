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

#ifndef _TEXTUREFACTORY_H
#define _TEXTUREFACTORY_H

#include "contentfactory.h"
#include "textureinfo.h"

class TEXTURE;

template <>
class Factory<TEXTURE>
{
public:
	struct empty {};

	Factory();

	/// in general all textures on disk will be in the SRGB colorspace, so if the renderer wants to do
	/// gamma correct lighting, it will want all textures to be gamma corrected using the SRGB flag
	/// limit texture size to max size
	void init(int max_size, bool use_srgb);

	template <class P>
	bool create(
		std::tr1::shared_ptr<TEXTURE> & sptr,
		std::ostream & error,
		const std::string & basepath,
		const std::string & path,
		const std::string & name,
		const P & param);

	std::tr1::shared_ptr<TEXTURE> getDefault() const;

private:
	std::tr1::shared_ptr<TEXTURE> m_default;
	int m_size;
	bool m_srgb;
};

#endif // _TEXTUREFACTORY_H
