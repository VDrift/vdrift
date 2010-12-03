#ifndef _FONT_H
#define _FONT_H

#include <vector>
#include <string>
#include <iostream>
#include <tr1/memory>

class TEXTUREMANAGER;
class TEXTURE;

class FONT
{
public:
	FONT() : charinfo(charnum) {};
	
	struct CHARINFO
	{
		CHARINFO() : loaded(false) {}
		bool loaded;
		float x, y, width, height;
		float xoffset, yoffset, xadvance;
	};
	
	bool Load(
		const std::string & fontinfopath,
		const std::string & texpath,
		const std::string & texname,
		const std::string & texsize,
		TEXTUREMANAGER & textures,
		std::ostream & error_output,
		bool mipmap = false);
	
	const std::tr1::shared_ptr<TEXTURE> GetFontTexture() const
	{
		return font_texture;
	}
	
	///returns the charinfo or nothing if the character is out of range
	bool GetCharInfo(char c, const CHARINFO * & info) const
	{
		unsigned int i = *((unsigned char*)&c);
		if (i < charnum && charinfo[i].loaded)
		{
			info = &charinfo[i];
			return true;
		}
		return false;
	}
	
	float GetWidth(const std::string & newtext, const float newscale) const;
	
private:
	std::tr1::shared_ptr<TEXTURE> font_texture;
	std::vector <CHARINFO> charinfo;
	static const unsigned int charnum = 256;
};

#endif
