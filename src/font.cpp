#include "font.h"

#include <fstream>
using std::ifstream;

#include <string>
using std::string;

#include <iostream>
using std::endl;

#include <algorithm>

bool FONT::Load(const std::string & fontinfopath, const std::string & fonttexturepath, const std::string & texsize, std::ostream & error_output, bool mipmap)
{
	ifstream fontinfo(fontinfopath.c_str());
	if (!fontinfo)
	{
		error_output << "Can't find font information file: " << fontinfopath << endl;
		return false;
	}
	
	string curstr;
	while (fontinfo && curstr != "chars")
		fontinfo >> curstr; //advance to first interesting bit
	
	fontinfo >> curstr;
	//if (!VerifyParse("count=96", curstr, fontinfopath, error_output)) return false;
	
	charinfo.resize(128);
	
	while (fontinfo.good())
	{
		fontinfo >> curstr;
		if (fontinfo.good())
		{
			if (!VerifyParse("char", curstr, fontinfopath, error_output)) return false;
			
			unsigned int cur_id(0);
			if (!Parse("id=", cur_id, fontinfo,fontinfopath, error_output)) return false;
			if (cur_id >= charinfo.size())
			{
				error_output << "Font info file " << fontinfopath << ": ID is out of range: " << cur_id << endl;
				return false;
			}
			//std::cout << "Parsing ID " << cur_id << endl;
			
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
	
	TEXTUREINFO texinfo(fonttexturepath);
	texinfo.SetMipMap(mipmap);
	texinfo.SetRepeat(false, false);
	if (!font_texture.Load(texinfo, error_output, texsize)) return false;
	
	if (font_texture.GetScale() != 1.0)
	{
		for (std::vector <CHARINFO>::iterator i = charinfo.begin(); i != charinfo.end(); ++i)
		{
			CHARINFO & char_to_scale = *i;
			char_to_scale.x *= font_texture.GetScale();
			char_to_scale.y *= font_texture.GetScale();
			char_to_scale.width *= font_texture.GetScale();
			char_to_scale.height *= font_texture.GetScale();
			char_to_scale.xoffset *= font_texture.GetScale();
			char_to_scale.yoffset *= font_texture.GetScale();
			char_to_scale.xadvance *= font_texture.GetScale();
		}
	}
	
	return true;
}

bool FONT::VerifyParse(const std::string & expected, const std::string & actual, const std::string & fontinfopath, std::ostream & error_output) const
{
	if (expected != actual)
	{
		error_output << "Font info file " << fontinfopath << ": expected " << expected << ", got " << actual << endl;
		return false;
	}
	else
		return true;
}
