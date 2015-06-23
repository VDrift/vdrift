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

#ifndef _FONT_H
#define _FONT_H

#include <memory>
#include <vector>
#include <iosfwd>
#include <string>

class ContentManager;
class Texture;

class Font
{
public:
	Font() : charinfo(charnum), inv_size(256.0/40.0) {};

	struct CharInfo
	{
		CharInfo() : loaded(false) {}
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

	const std::shared_ptr<Texture> & GetFontTexture() const
	{
		return font_texture;
	}

	bool GetCharInfo(char c, const CharInfo * & info) const
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
	std::shared_ptr<Texture> font_texture;	// font texture
	std::vector <CharInfo> charinfo;			// font metrics in texture space
	float inv_size;								// inverse font size in texture space
	static const unsigned int charnum = 256;	// character count
};

#endif
