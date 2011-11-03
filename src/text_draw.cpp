#include "text_draw.h"

float TEXT_DRAW::RenderCharacter(
	const FONT & font, char c,
	float x, float y, float scalex, float scaley,
	VERTEXARRAY & output_array)
{
	const FONT::CHARINFO * ci = 0;
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

	float v[] = {x1, y1, 0, x2, y1, 0, x2, y2, 0, x1, y2, 0};
	float t[] = {u1, v1, u2, v1, u2, v2, u1, v2};
	int f[] = {0, 1, 2, 0, 2, 3};

	if (output_array.GetTexCoordSets() == 0)
	{
		output_array.SetFaces(f, 6);
		output_array.SetVertices(v, 12);
		output_array.SetTexCoordSets(1);
		output_array.SetTexCoords(0, t, 8);
	}
	else
	{
		float * n = 0;
		output_array.Add(n, 0, v, 12, f, 6, t, 8);
	}

	return ci->xadvance * invsize * scalex;
}

float TEXT_DRAW::RenderText(
	const FONT & font, const std::string & text,
	float x, float y, float scalex, float scaley,
	VERTEXARRAY & output_array)
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

void TEXT_DRAW::SetText(
	DRAWABLE & draw,
	const FONT & font, const std::string & text,
	float x, float y, float scalex, float scaley,
	float r, float g, float b,
	VERTEXARRAY & output_array)
{
	RenderText(font, text, x, y, scalex, scaley, output_array);
	draw.SetDiffuseMap(font.GetFontTexture());
	draw.SetVertArray(&output_array);
	draw.SetCull(false, false);
	draw.SetColor(r, g, b, 1.0);
}

TEXT_DRAW::TEXT_DRAW() : oldx(0), oldy(0), oldscalex(1), oldscaley(1)
{
	// ctor
}

void TEXT_DRAW::Set(
	DRAWABLE & draw,
	const FONT & font, const std::string & newtext,
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

void TEXT_DRAW::Revise(
	const FONT & font, const std::string & newtext,
	float x, float y, float scalex, float scaley)
{
	RenderText(font, newtext, x, y, scalex, scaley, varray);
	text = newtext;
	oldx = x;
	oldy = y;
	oldscalex = scalex;
	oldscaley = scaley;
}

void TEXT_DRAW::Revise(const FONT & font, const std::string & newtext)
{
	Revise(font, newtext, oldx, oldy, oldscalex, oldscaley);
	text = newtext;
}
