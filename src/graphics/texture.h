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

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "texture_interface.h"
#include "textureinfo.h"

#include <iosfwd>
#include <string>

class Texture : public TextureInterface
{
public:
	Texture();

	virtual ~Texture();

	bool Load(const std::string & path, const TextureInfo & info, std::ostream & error);

	void Unload();

private:
	bool LoadCubeVerticalCross(const std::string & path, const TextureInfo & info, std::ostream & error);

	bool LoadCube(const std::string & path, const TextureInfo & info, std::ostream & error);

	bool LoadDDS(const std::string & path, const TextureInfo & info, std::ostream & error);
};

#endif //_TEXTURE_H

