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

struct TextureInfo
{
	enum Size { SMALL, LARGE, MEDIUM };
	unsigned char* data;	///< raw data pointer
	short width;			///< texture width, only set if data not null
	short height;			///< texture height, only set if data not null
	char bytespp;			///< bytes per pixel, only set if data not null
	char anisotropy;		///< anisotropic filter level
	Size maxsize;			///< max texture size 128, 256, 2048
	bool mipmap;			///< build mip maps
	bool cube;				///< is a cube map
	bool verticalcross; 	///< is a vertical cross cube map
	bool compress;			///< can be compressed (not a normal map e.g.)
	bool repeatu;			///< repeat texture along u coordinate
	bool repeatv;			///< repeat texture along v coordinate
	bool npot;				///< is not power of two
	bool nearest;			///< use nearest-neighbor interpolation filter
	bool premultiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	bool srgb; 				///< apply srgb colorspace correction

	TextureInfo() :
		data(0),
		width(0),
		height(0),
		bytespp(4),
		anisotropy(0),
		maxsize(LARGE),
		mipmap(true),
		cube(false),
		verticalcross(false),
		compress(true),
		repeatu(true),
		repeatv(true),
		npot(true),
		nearest(false),
		premultiply_alpha(false),
		srgb(false)
	{
		// ctor
	}
};

#endif // _TEXTUREINFO_H
