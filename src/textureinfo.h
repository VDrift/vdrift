#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

struct TEXTUREINFO
{
	enum Size { SMALL, LARGE, MEDIUM };
	char* data;				///< raw data pointer
	short width;			///< texture width, only set if data not null
	short height;			///< texture height, only set if data not null
	char bytespp;			///< bytes per pixel, only set if data not null
	char anisotropy;		///< anisotropic filter level
	Size maxsize;			///< max texture size 128, 256, 2048
	bool mipmap;			///< build mip maps
	bool cube;				///< is a cube map
	bool verticalcross; 	///< is a vertical cross cube map
	bool normalmap;			///< is a normal map
	bool repeatu;			///< repeat texture along u coordinate
	bool repeatv;			///< repeat texture along v coordinate
	bool npot;				///< is not power of two
	bool nearest;			///< use nearest-neighbor interpolation filter
	bool premultiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	bool srgb; 				///< apply srgb colorspace correction

	TEXTUREINFO() :
		data(0),
		width(0),
		height(0),
		bytespp(4),
		anisotropy(0),
		maxsize(LARGE),
		mipmap(true),
		cube(false),
		verticalcross(false),
		normalmap(false),
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
