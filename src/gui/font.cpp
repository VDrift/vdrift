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

#include "font.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"

#include <sstream>
#include <fstream>

template <typename T>
static void GetNumber(const std::string & numberstr, T & num_output)
{
	std::istringstream s(numberstr);
	s >> num_output;
}

static bool VerifyParse(
	const std::string & expected,
	const std::string & actual,
	const std::string & fontinfopath,
	std::ostream & error_output)
{
	if (expected != actual)
	{
		error_output << "Font info file " << fontinfopath << ": expected " << expected << ", got " << actual << std::endl;
		return false;
	}
	else
	{
		return true;
	}
}

template <typename T>
static bool Parse(
	const std::string & parameter,
	T & num_output,
	std::ifstream & f,
	const std::string & fontinfopath,
	std::ostream & error_output)
{
	std::string curstr;
	f >> curstr;
	if (!VerifyParse(parameter, curstr.substr(0,parameter.size()), fontinfopath, error_output)) return false;
	GetNumber(curstr.substr(parameter.size()), num_output);
	return true;
}

bool Font::Load(
	const std::string & fontinfopath,
	const std::string & texpath,
	const std::string & texname,
	ContentManager & content,
	std::ostream & error_output,
	bool mipmap)
{
	TextureInfo texinfo;
	texinfo.mipmap = mipmap;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	content.load(font_texture, texpath, texname, texinfo);

	std::ifstream fontinfo(fontinfopath.c_str());
	if (!fontinfo)
	{
		error_output << "Can't find font information file: " << fontinfopath << std::endl;
		return false;
	}

	const std::string scalestr("scale=");
	const std::string sizestr("size=");
	float font_scale = 256;
	float font_size = 40;
	while (fontinfo)
	{
		std::string curstr;
		fontinfo >> curstr;
		if (curstr == "char")
		{
			unsigned int cur_id(0);
			if (!Parse("id=", cur_id, fontinfo, fontinfopath, error_output)) return false;

			CharInfo & info = charinfo[cur_id];
			if (!Parse("x=", info.x, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("y=", info.y, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("width=", info.width, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("height=", info.height, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("xoffset=", info.xoffset, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("yoffset=", info.yoffset, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("xadvance=", info.xadvance, fontinfo, fontinfopath, error_output)) return false;
			fontinfo >> curstr >> curstr; //don't care
			info.loaded = true;
		}
		else if (curstr.compare(0, scalestr.size(), scalestr) == 0)
		{
			if (!VerifyParse(scalestr, curstr.substr(0, scalestr.size()), fontinfopath, error_output)) return false;
			GetNumber(curstr.substr(scalestr.size()), font_scale);
		}
		else if (curstr.compare(0, sizestr.size(), sizestr) == 0)
		{
			if (!VerifyParse(sizestr, curstr.substr(0, sizestr.size()), fontinfopath, error_output)) return false;
			GetNumber(curstr.substr(sizestr.size()), font_size);
		}
	}

	const float inv_scale = 1 / font_scale;
	for (size_t i = 0; i < charinfo.size(); ++i)
	{
		CharInfo & info = charinfo[i];
		info.x *= inv_scale;
		info.y *= inv_scale;
		info.width *= inv_scale;
		info.height *= inv_scale;
		info.xoffset *= inv_scale;
		info.yoffset *= inv_scale;
		info.xadvance *= inv_scale;
	}

	inv_size = font_scale / font_size;

	return true;
}

float Font::GetWidth(const std::string & newtext) const
{
	float cursorx(0);
	float linewidth(0);
	for (unsigned int i = 0; i < newtext.size(); ++i)
	{
		if (newtext[i] == '\n')
		{
			if (linewidth < cursorx) linewidth = cursorx;
			cursorx = 0;
		}
		else
		{
			const CharInfo * cinfo(0);
			if (GetCharInfo(newtext[i], cinfo))
			{
				cursorx += cinfo->xadvance * inv_size;
			}
		}
	}
	if (linewidth < cursorx) linewidth = cursorx;
	return linewidth;
}
