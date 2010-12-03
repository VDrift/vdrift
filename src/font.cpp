#include "font.h"
#include "texturemanager.h"

#include <sstream>
#include <fstream>
#include <algorithm>

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
	const std::string & texsize,
	TEXTUREMANAGER & textures,
	std::ostream & error_output,
	bool mipmap)
{
	std::ifstream fontinfo(fontinfopath.c_str());
	if (!fontinfo)
	{
		error_output << "Can't find font information file: " << fontinfopath << std::endl;
		return false;
	}
	
	std::string curstr;
	while (fontinfo && curstr != "chars") fontinfo >> curstr; //advance to first interesting bit
	
	fontinfo >> curstr;
	while (fontinfo.good())
	{
		fontinfo >> curstr;
		if (fontinfo.good())
		{
			if (!VerifyParse("char", curstr, fontinfopath, error_output)) return false;
			
			unsigned int cur_id(0);
			if (!Parse("id=", cur_id, fontinfo, fontinfopath, error_output)) return false;
			if (!Parse("x=", charinfo[cur_id].x, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("y=", charinfo[cur_id].y, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("width=", charinfo[cur_id].width, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("height=", charinfo[cur_id].height, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("xoffset=", charinfo[cur_id].xoffset, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("yoffset=", charinfo[cur_id].yoffset, fontinfo,fontinfopath, error_output)) return false;
			if (!Parse("xadvance=", charinfo[cur_id].xadvance, fontinfo,fontinfopath, error_output)) return false;
			
			fontinfo >> curstr >> curstr; //don't care
			
			charinfo[cur_id].loaded = true;
		}
	}
	
	TEXTUREINFO texinfo;
	texinfo.mipmap = mipmap;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	texinfo.size = texsize;
	if (!textures.Load(texpath, texname, texinfo, font_texture)) return false;
	
	float scale = font_texture->GetScale();
	if (scale != 1.0)
	{
		for (std::vector <CHARINFO>::iterator i = charinfo.begin(); i != charinfo.end(); ++i)
		{
			CHARINFO & char_to_scale = *i;
			char_to_scale.x *= scale;
			char_to_scale.y *= scale;
			char_to_scale.width *= scale;
			char_to_scale.height *= scale;
			char_to_scale.xoffset *= scale;
			char_to_scale.yoffset *= scale;
			char_to_scale.xadvance *= scale;
		}
	}
	
	return true;
}

float FONT::GetWidth(const std::string & newtext, const float newscale) const
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
				cursorx += (cinfo->xadvance / font_texture->GetW()) * newscale;
			}
		}
	}
	if (linewidth < cursorx) linewidth = cursorx;
	return linewidth;
}
