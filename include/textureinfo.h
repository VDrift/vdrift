#ifndef _TextureInfo_h
#define _TextureInfo_h

#include <string>

class SDL_Surface;

struct TextureInfo
{
	std::string name;
	SDL_Surface * surface;
	unsigned char anisotropy;
	enum {large, medium, small} size;
	bool mipmap;
	bool cube;
	bool verticalcross;
	bool normalmap;
	bool repeatu;
	bool repeatv;
	bool npot;
	bool nearest;
	bool premultiplied_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	
	TextureInfo() :
		surface(NULL),
		anisotropy(0),
		size(large),
		mipmap(true),
		cube(false),
		verticalcross(false),
		normalmap(false),
		repeatu(true),
		repeatv(true),
		npot(true),
		nearest(false),
		premultiplied_alpha(false)
	{
		//ctor
	}
	
	virtual ~TextureInfo()
	{
		//dtor
	}
	
	float getScale(float width, float height) const
	{
		float scale = 1.0;
		if (size == medium || size == small)
		{
			float maxsize = 256;
			float minscale = 0.5;
			if (size == small)
			{
				maxsize = 128;
				minscale = 0.25;
			}
			float scalew = 1.0;
			float scaleh = 1.0;

			if (width > maxsize)
				scalew = maxsize / width;
			if (height > maxsize)
				scaleh = maxsize / height;

			if (scalew < scaleh)
				scale = scalew;
			else
				scale = scaleh;

			if (scale < minscale)
				scale = minscale;
		}
		return scale;
	}
	
	void setSize(const std::string & value)
	{
		if (value == "medium")
			size = medium;
		else if (value == "small")
			size = small;
		else
			size = large;
	}
	
	friend std::ostream & operator << (std::ostream &os, const TextureInfo & t)
	{
		os << t.name;
		return os;
	}
};

#endif // _TextureInfo_h
