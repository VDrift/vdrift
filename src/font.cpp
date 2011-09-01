#include "font.h"
#include "contentmanager.h"
#include "texture.h"

#include <sstream>
#include <fstream>

template <typename T>
static void GetNumber(const std::string & numberstr, T & num_output)
{
	std::stringstream s(numberstr);
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

bool FONT::Load(
	const std::string & fontinfopath,
	const std::string & texpath,
	const std::string & texname,
	ContentManager & content,
	std::ostream & error_output,
	bool mipmap)
{
	TEXTUREINFO texinfo;
	texinfo.mipmap = mipmap;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	if (!content.load(texpath, texname, texinfo, font_texture)) return false;

	std::ifstream fontinfo(fontinfopath.c_str());
	if (!fontinfo)
	{
		error_output << "Can't find font information file: " << fontinfopath << std::endl;
		return false;
	}

	const std::string sizestr("size=");
	const float sw = font_texture->GetScale() / font_texture->GetW();
	const float sh = font_texture->GetScale() / font_texture->GetH();
	while (fontinfo)
	{
		std::string curstr;
		fontinfo >> curstr;
		if (curstr == "char")
		{
			unsigned int cur_id(0);
			if (!Parse("id=", cur_id, fontinfo, fontinfopath, error_output)) return false;

			CHARINFO & info = charinfo[cur_id];
			if (!Parse("x=", info.x, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("y=", info.y, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("width=", info.width, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("height=", info.height, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("xoffset=", info.xoffset, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("yoffset=", info.yoffset, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("xadvance=", info.xadvance, fontinfo, fontinfopath, error_output)) return false;
			fontinfo >> curstr >> curstr; //don't care

			info.x *= sw;
			info.y *= sh;
			info.width *= sw;
			info.height *= sh;
			info.xoffset *= sw;
			info.yoffset *= sh;
			info.xadvance *= sw;
			info.loaded = true;
		}
		else if (curstr.compare(0, sizestr.size(), sizestr) == 0)
		{
			float size(0);
			if (!VerifyParse(sizestr, curstr.substr(0, sizestr.size()), fontinfopath, error_output)) return false;
			GetNumber(curstr.substr(sizestr.size()), size);
			inv_size = 1 / (size * sh);
		}
	}

	return true;
}

float FONT::GetWidth(const std::string & newtext) const
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
			const CHARINFO * cinfo(0);
			if (GetCharInfo(newtext[i], cinfo))
			{
				cursorx += cinfo->xadvance * inv_size;
			}
		}
	}
	if (linewidth < cursorx) linewidth = cursorx;
	return linewidth;
}
