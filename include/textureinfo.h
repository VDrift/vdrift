#ifndef _TEXTUREINFO_H
#define _TEXTUREINFO_H

#include <string>

class SDL_Surface;

class TEXTUREINFO
{
public:
	TEXTUREINFO()
	{
		Init();
	}
	
	TEXTUREINFO(const std::string & newname) : name(newname)
	{
		Init();
	}

	const std::string GetName() const
	{
		return name;
	}
	
	bool GetMipMap() const
	{
		return mipmap;
	}
	
	bool GetCube() const
	{
		return cube;
	}
	
	bool GetVerticalCross() const
	{
		return cube && verticalcross;
	}
	
	bool NormalMap() const
	{
		return normalmap;
	}
	
	void CopyFrom(const TEXTUREINFO & other)
	{
		*this = other;
	}
	
	void SetName(const std::string & newname)
	{
		name = newname;
	}
	
	void SetCube(const bool newcube, const bool newvertcross)
	{
		cube = newcube;
		verticalcross = newvertcross;
	}
	
	void SetNormalMap(const bool newnorm)
	{
		normalmap = newnorm;
	}
	
	void SetMipMap(const bool newmipmap)
	{
		mipmap = newmipmap;
	}
	
	void SetSurface(SDL_Surface * value)
	{
		surface = value;
	}
	
	SDL_Surface * GetSurface() const
	{
		return surface;
	}
	
	int GetAnisotropy() const
	{
		return anisotropy;
	}
	
	void SetAnisotropy(int value)
	{
		anisotropy = value;
	}
	
	void SetRepeat(bool u, bool v)
	{
		repeatu = u;
		repeatv = v;
	}

	bool GetRepeatU() const
	{
		return repeatu;
	}

	bool GetRepeatV() const
	{
		return repeatv;
	}

	void SetAllowNonPowerOfTwo(bool allow)
	{
	    allow_non_power_of_two = allow;
	}
	
	bool GetAllowNonPowerOfTwo() const
	{
	    return allow_non_power_of_two;
	}

	void SetNearest(bool value)
	{
		nearest = value;
	}

	bool GetNearest() const
	{
		return nearest;
	}

private:
	std::string name;
	bool mipmap;
	bool cube;
	bool verticalcross;
	bool normalmap;
	SDL_Surface * surface;
	int anisotropy;
	bool repeatu, repeatv;
	bool allow_non_power_of_two;
	bool nearest;
	bool pre_multiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending

	void Init()
	{
		mipmap = true;
		cube = false;
		verticalcross = false;
		normalmap = false;
		surface = NULL;
		anisotropy = 0;
		repeatu = true;
		repeatv = true;
		allow_non_power_of_two = true;
		nearest = false;
	}
};

#endif // _TEXTUREINFO_H
