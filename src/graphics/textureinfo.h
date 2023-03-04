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

#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

struct TextureData
{
	unsigned char* data = 0;	///< raw data pointer
	unsigned width = 0;			///< texture width, only set if data not null
	unsigned height = 0;		///< texture height, only set if data not null
	unsigned char bytespp = 0;	///< bytes per pixel, only set if data not null
};

struct TextureInfo
{
	enum Size { LARGE=0, MEDIUM=1, SMALL=2 };
	char maxsize = LARGE;			///< max texture size 128, 256, 2048
	char anisotropy = 0;			///< anisotropic filter level
	bool mipmap = true;				///< build mip maps
	bool cube = false;				///< is a cube map
	bool compress = true;			///< can be compressed (not a normal map e.g.)
	bool repeatu = true;			///< repeat texture along u coordinate
	bool repeatv = true;			///< repeat texture along v coordinate
	bool nearest = false;			///< use nearest-neighbor interpolation filter
	bool premultiply_alpha = false; ///< pre-multiply the color by the alpha value; allows using glstate.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	bool srgb = false;				///< apply srgb colorspace correction
};

#endif // _TEXTUREINFO_H
