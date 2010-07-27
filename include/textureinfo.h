#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

#include <string>

class SDL_Surface;

struct TextureInfo
{
	TextureInfo() :
		surface(NULL),
		anisotropy(0),
		mipmap(true),
		normalmap(false),
		repeatu(true),
		repeatv(true),
		npot(true),
		nearest(false),
		premultiplied_alpha(false)
	{
		//ctor
	}

	std::string name;
	std::string size;           ///< "medium" or "small" else no scaling applied
	SDL_Surface * surface;
	unsigned char anisotropy;
	bool mipmap;
	bool normalmap;
	bool repeatu;
	bool repeatv;
	bool npot;
	bool nearest;
	bool premultiplied_alpha;   ///< use glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) for blending

};

#endif // _TEXTUREINFO_H
