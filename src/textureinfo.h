#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

#include <string>

struct SDL_Surface;

struct TEXTUREINFO
{
	std::string size;
	SDL_Surface * surface;
	unsigned char anisotropy;
	bool mipmap;
	bool cube;
	bool verticalcross;
	bool normalmap;
	bool repeatu;
	bool repeatv;
	bool npot;
	bool nearest;
	bool premultiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	bool srgb; ///< whether or not to do srgb colorspace correction for this texture

	TEXTUREINFO() :
		size("large"),
		surface(0),
		anisotropy(0),
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
