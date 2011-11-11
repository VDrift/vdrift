#ifndef _FONT_H
#define _FONT_H

#include "memory.h"
#include <vector>
#include <string>
#include <iostream>

class ContentManager;
class TEXTURE;

class FONT
{
public:
	FONT() : charinfo(charnum), inv_size(256.0/40.0) {};

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
		ContentManager & content,
		std::ostream & error_output,
		bool mipmap = false);

	const std::tr1::shared_ptr<TEXTURE> GetFontTexture() const
	{
		return font_texture;
	}

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

	// get normalized(font height = 1) string width
	float GetWidth(const std::string & newtext) const;

	float GetInvSize() const {return inv_size;}

private:
	std::tr1::shared_ptr<TEXTURE> font_texture;	// font texture
	std::vector <CHARINFO> charinfo;			// font metrics in texture space
	float inv_size;								// inverse font size in texture space
	static const unsigned int charnum = 256;	// character count
};

#endif
