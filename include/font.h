#ifndef _FONT_H
#define _FONT_H

#include "optional.h"

#include <vector>
#include <string>
#include <ostream>
#include <tr1/memory>

class TEXTUREMANAGER;
class TEXTURE;

class FONT
{
public:
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
	optional <const CHARINFO *> GetCharInfo(unsigned int id) const
	{
		if (id < charinfo.size() && charinfo[id].loaded)
			return &charinfo[id];
		else
			return optional <const CHARINFO *> ();
	}
	
	float GetWidth(const std::string & newtext, const float newscale) const;
	
private:
	std::tr1::shared_ptr<TEXTURE> font_texture;
	std::vector <CHARINFO> charinfo;
};

#endif
