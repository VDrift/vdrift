#include "text_draw.h"
#include "texture.h"

#include <iostream>
#include <algorithm>

void TEXT_DRAW::Set(DRAWABLE & draw, const FONT & font, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley, const float r, const float g, const float b, VERTEXARRAY & output_array)
{
	output_array.Clear();
	
	text = newtext;
	draw.SetDiffuseMap(font.GetFontTexture());
	draw.SetVertArray(&output_array);
	draw.SetCull(false, false);
	draw.SetColor(r,g,b,1.0);
	
	float cursorx = x;
	float cursory = y;
	
	for (unsigned int i = 0; i < newtext.size(); i++)
	{
		if (text[i] == '\n')
		{
			cursorx = x;
			cursory += newscaley;
		}
		else
		{
			optional <const FONT::CHARINFO *> cinfo = font.GetCharInfo(text[i]);
			if (cinfo)
			{
				cursorx += RenderCharacter(output_array, font.GetFontTexture()->GetW(), font.GetFontTexture()->GetH(), cursorx, cursory, newscalex, newscaley, *cinfo.get());
			}
		}
	}
	
	//std::cout << output_array.GetNumFaces() << std::endl;
	
	oldx = x;
	oldy = y;
	oldscalex = newscalex;
	oldscaley = newscaley;
}

void TEXT_DRAW::Revise(const FONT & font, const std::string & newtext, float x, float y, float scalex, float scaley, VERTEXARRAY & output_array)
{
	text = newtext;
	output_array.Clear();
	float cursorx = x;
	float cursory = y;
	
	for (unsigned int i = 0; i < newtext.size(); i++)
	{
		if (newtext[i] == '\n')
		{
			cursorx = x;
			cursory += scaley*0.15;
		}
		else
		{
			optional <const FONT::CHARINFO *> cinfo = font.GetCharInfo(text[i]);
			if (cinfo)
			{
				cursorx += RenderCharacter(output_array, font.GetFontTexture()->GetW(), font.GetFontTexture()->GetH(), cursorx, cursory, scalex, scaley, *cinfo.get());
			}
		}
	}
	
	oldx = x;
	oldy = y;
	oldscalex = scalex;
	oldscaley = scaley;
}

float TEXT_DRAW::RenderCharacter(VERTEXARRAY & output_array, const float tw, const float th, const float x, const float y, const float scalex, const float scaley, const FONT::CHARINFO & c)
{
	//float x1, y1, x2, y2;
	//float u1, v1, u2, v2;
	
	float x1 = x + (float)c.xoffset / tw * scalex;
	float x2 = x1 + (float)c.width / tw * scalex;
	float y1 = y - (float)c.yoffset / th * scaley;
	float y2 = y1 + (float)c.height / th * scaley;
	
	float u1 = (float)c.x / tw;
	float u2 = u1 + (float)c.width / tw;
	float v1 = (float)c.y / th;
	float v2 = v1 + (float)c.height / th;
	
	//std::cout << x1 << "," << x2 << "," << y1 << "," << y2 << std::endl;
	//std::cout << u1 << "," << u2 << "," << v1 << "," << v2 << std::endl;
	
	float vcorners[12];
	vcorners[0] = x1;
	vcorners[1] = y1;
	vcorners[2] = 0;
	vcorners[3] = x2;
	vcorners[4] = y1;
	vcorners[5] = 0;
	vcorners[6] = x2;
	vcorners[7] = y2;
	vcorners[8] = 0;
	vcorners[9] = x1;
	vcorners[10] = y2;
	vcorners[11] = 0;
	
	float uvs[8];
	uvs[0] = u1;
	uvs[1] = v1;
	uvs[2] = u2;
	uvs[3] = v1;
	uvs[4] = u2;
	uvs[5] = v2;
	uvs[6] = u1;
	uvs[7] = v2;
	
	int bfaces[6];
	bfaces[0] = 0;
	bfaces[1] = 1;
	bfaces[2] = 2;
	bfaces[3] = 0;
	bfaces[4] = 2;
	bfaces[5] = 3;
	
	if (output_array.GetTexCoordSets() == 0)
	{
		output_array.SetFaces(bfaces, 6);
		output_array.SetVertices(vcorners, 12);
		output_array.SetTexCoordSets(1);
		output_array.SetTexCoords(0, uvs, 8);
	}
	else
	{
		float * norms(NULL);
		output_array.Add(norms, 0, vcorners, 12, bfaces, 6, uvs, 8);
	}
	
	return ((float)c.xadvance/tw)*scalex;
}

float TEXT_DRAW::GetWidth(const FONT & font, const std::string & newtext, const float newscale)
{
	float cursorx(0);
	std::vector <float> linewidth;
	
	for (unsigned int i = 0; i < newtext.size(); i++)
	{
		if (newtext[i] == '\n')
		{
			linewidth.push_back(cursorx);
			cursorx = 0;
		}
		else
		{
			optional <const FONT::CHARINFO *> cinfo = font.GetCharInfo(newtext[i]);
			if (cinfo)
			{
				cursorx += (cinfo.get()->xadvance/font.GetFontTexture()->GetW())*newscale;
			}
		}
	}
	
	linewidth.push_back(cursorx);
	
	float maxwidth = *std::max_element(linewidth.begin(),linewidth.end());
	
	return maxwidth;
}
