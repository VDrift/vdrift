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

#include "text_draw.h"
#include "graphics/texture.h"

float TextDraw::RenderCharacter(
	const Font & font, char c,
	float x, float y, float scalex, float scaley,
	VertexArray & output_array)
{
	const Font::CharInfo * ci = 0;
	if (!font.GetCharInfo(c, ci)) return 0;

	float invsize = font.GetInvSize();
	float x1 = x + ci->xoffset * invsize * scalex;
	float x2 = x1 + ci->width * invsize * scalex;
	float y1 = y - ci->yoffset * invsize * scaley;
	float y2 = y1 + ci->height * invsize * scaley;

	float u1 = ci->x;
	float u2 = u1 + ci->width;
	float v1 = ci->y;
	float v2 = v1 + ci->height;

	const float v[] = {x1, y1, 0, x2, y1, 0, x2, y2, 0, x1, y2, 0};
	const float t[] = {u1, v1, u2, v1, u2, v2, u1, v2};
	const unsigned int f[] = {0, 1, 2, 0, 2, 3};

	output_array.Add(f, 6, v, 12, t, 8);

	return ci->xadvance * invsize * scalex;
}

float TextDraw::RenderText(
	const Font & font, const std::string & text,
	float x, float y, float scalex, float scaley,
	VertexArray & output_array)
{
	output_array.Clear();
	float cursorx = x;
	float cursory = y  + scaley / 4;
	for (unsigned int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '\n')
		{
			cursorx = x;
			cursory += scaley;
		}
		else
		{
			cursorx += RenderCharacter(font, text[i], cursorx, cursory, scalex, scaley, output_array);
		}
	}
	return cursorx;
}

void TextDraw::SetText(
	Drawable & draw,
	const Font & font, const std::string & text,
	float x, float y, float scalex, float scaley,
	float r, float g, float b,
	VertexArray & output_array)
{
	RenderText(font, text, x, y, scalex, scaley, output_array);
	draw.SetTextures(font.GetFontTexture()->GetId());
	draw.SetVertArray(&output_array);
	draw.SetCull(false);
	draw.SetColor(r, g, b, 1.0);
}

TextDraw::TextDraw() :
	oldx(0),
	oldy(0),
	oldscalex(1),
	oldscaley(1)
{
	// ctor
}

void TextDraw::Set(
	Drawable & draw,
	const Font & font, const std::string & newtext,
	float x,  float y, float newscalex, float newscaley,
	float r, float g, float b)
{
	SetText(draw, font, newtext, x, y, newscalex, newscaley, r, g, b, varray);
	text = newtext;
	oldx = x;
	oldy = y;
	oldscalex = newscalex;
	oldscaley = newscaley;
}

void TextDraw::Revise(
	const Font & font, const std::string & newtext,
	float x, float y, float scalex, float scaley)
{
	RenderText(font, newtext, x, y, scalex, scaley, varray);
	text = newtext;
	oldx = x;
	oldy = y;
	oldscalex = scalex;
	oldscaley = scaley;
}

void TextDraw::Revise(const Font & font, const std::string & newtext)
{
	Revise(font, newtext, oldx, oldy, oldscalex, oldscaley);
	text = newtext;
}
