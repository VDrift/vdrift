#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

#include <string>

class SDL_Surface;

class TEXTUREINFO
{
public:
	TEXTUREINFO() {Init();}
	
	TEXTUREINFO(const std::string & newname) {name = newname; Init();}
	
	virtual ~TEXTUREINFO() {}

	const std::string GetName() const {return name;}
	void SetName(const std::string & newname) {name = newname;}
	
	SDL_Surface * GetSurface() const {return surface;}
	void SetSurface(SDL_Surface * value) {surface = value;}
	
	int GetAnisotropy() const {return anisotropy;}
	void SetAnisotropy(unsigned int value) {anisotropy = value;}
	
	bool GetMipMap() const {return mipmap;}
	void SetMipMap(const bool newmipmap) {mipmap = newmipmap;}
	
	bool GetCube() const {return cube;}
	bool GetVerticalCross() const {return cube && verticalcross;}
	void SetCube(const bool newcube, const bool newvertcross) {cube = newcube; verticalcross = newvertcross;}
	
	bool NormalMap() const {return normalmap;}
	void SetNormalMap(const bool newnorm) {normalmap = newnorm;}

	bool GetRepeatU() const {return repeatu;}
	bool GetRepeatV() const {return repeatv;}
	void SetRepeat(bool u, bool v) {repeatu = u; repeatv = v;}

	bool GetAllowNonPowerOfTwo() const {return allow_non_power_of_two;}
	void SetAllowNonPowerOfTwo(bool allow) {allow_non_power_of_two = allow;}
	
	bool GetNearest() const {return nearest;}
	void SetNearest(bool value) {nearest = value;}

	float GetScale(float width, float height) const
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
	
	void SetSize(const std::string & value)
	{
		if (value == "medium")
			size = medium;
		else if (value == "small")
			size = small;
	}

private:
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
	bool allow_non_power_of_two;
	bool nearest;
	bool pre_multiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending
	
	void Init()
	{
		surface = NULL;
		anisotropy = 0;
		size = large;
		mipmap = true;
		cube = false;
		verticalcross = false;
		normalmap = false;
		repeatu = true;
		repeatv = true;
		allow_non_power_of_two = true;
		nearest = false;
		pre_multiply_alpha = false;
	}
};

#endif // _TEXTUREINFO_H
