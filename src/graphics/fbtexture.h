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

#ifndef _FBTEXTURE_H
#define _FBTEXTURE_H

#include "texture_interface.h"
#include "glcore.h"

#include <iosfwd>
#include <cassert>

class FrameBufferTexture : public TextureInterface
{
public:
	enum Target
	{
		NORMAL = GL_TEXTURE_2D,
		RECTANGLE = GL_TEXTURE_RECTANGLE,
		CUBEMAP = GL_TEXTURE_CUBE_MAP
	};

	enum CubeSide
	{
		POSX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		NEGX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		POSY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		NEGY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		POSZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		NEGZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	enum Format
	{
		R8 = GL_R8,
		RGB8 = GL_RGB,
		RGBA8 = GL_RGBA,
		RGB16 = GL_RGB16,
		RGBA16 = GL_RGBA16,
		DEPTH24 = GL_DEPTH_COMPONENT24
	};

	FrameBufferTexture();

	~FrameBufferTexture();

	void Init(
		int sizex, int sizey,
		Target newtarget, Format newformat,
		bool filternearest, bool usemipmap,
		std::ostream & error_output,
		int newmultisample = 0,
		bool newdepthcomparisonenabled = true);

	void DeInit();

	void SetAttachment(int value) { attachment = value; }

	int GetAttachment() const { return attachment; }

	void SetSide(CubeSide side) { assert(target == CUBEMAP); cur_side = side; }

	CubeSide GetSide() const { return cur_side; }

	Format GetFormat() const { return format; }

	int GetMultiSample() const { return multisample; }

	bool HasMipMap() const { return mipmap; }

	bool Loaded() const { return inited; }

private:
	bool inited;
	bool mipmap;
	int multisample;
	int attachment;
	Format format;
	CubeSide cur_side;
	bool depthcomparisonenabled;
};

#endif
