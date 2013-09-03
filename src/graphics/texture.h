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

class Texture : public TextureInterface
{
public:
	Texture();

	virtual ~Texture();

	virtual void Activate() const;

	virtual void Deactivate() const;

	virtual bool Loaded() const {return m_id;}

	virtual unsigned GetW() const {return m_w;}

	virtual unsigned GetH() const {return m_h;}

	virtual unsigned GetID() const {return m_id;}

	/// scale factor from original size.  allows the user to determine
	/// what the texture size scaling did to the texture dimensions
	float GetScale() const {return m_scale;}

	bool IsCube() const {return m_cube;}

	bool Load(const std::string & path, const TextureInfo & info, std::ostream & error);

	void Unload();

private:
	unsigned m_id;
	unsigned m_w, m_h;	///< w and h are post-texture-size transform
	float m_scale;		///< amount of scaling applied by the texture-size transform
	bool m_alpha;
	bool m_cube;

	bool LoadCubeVerticalCross(const std::string & path, const TextureInfo & info, std::ostream & error);

	bool LoadCube(const std::string & path, const TextureInfo & info, std::ostream & error);

	bool LoadDDS(const std::string & path, const TextureInfo & info, std::ostream & error);
};

#endif //_TEXTURE_H

