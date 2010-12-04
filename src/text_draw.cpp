#include "text_draw.h"
#include "texture.h"

static float RenderCharacter(
	VERTEXARRAY & output_array,
	const float x,
	const float y,
	const float scalex,
	const float scaley,
	const FONT & font,
	const char c)
{
	const FONT::CHARINFO * ci(0);
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
		float * norms(0);
		output_array.Add(norms, 0, vcorners, 12, bfaces, 6, uvs, 8);
	}
	
	//output_array.SetTo2DQuad(x1, y1, x2, y2, u1, v1, u2, v2, 0);
	return ci->xadvance * invsize * scalex;
}

void TEXT_DRAW::Set(DRAWABLE & draw, const FONT & font, const std::string & newtext, const float x, const float y, const float scalex, const float scaley, const float r, const float g, const float b, VERTEXARRAY & output_array)
{
	Revise(font, newtext, x, y, scalex, scaley, output_array);
	draw.SetDiffuseMap(font.GetFontTexture());
	draw.SetVertArray(&output_array);
	draw.SetCull(false, false);
	draw.SetColor(r, g, b, 1.0);
}

void TEXT_DRAW::Revise(const FONT & font, const std::string & newtext, float x, float y, float scalex, float scaley, VERTEXARRAY & output_array)
{
	text = newtext;
	output_array.Clear();
	float cursorx = x;
	float cursory = y  + scaley / 4;
	for (unsigned int i = 0; i < newtext.size(); i++)
	{
		if (newtext[i] == '\n')
		{
			cursorx = x;
			cursory += scaley;
		}
		else
		{
			cursorx += RenderCharacter(output_array, cursorx, cursory, scalex, scaley, font, text[i]);
		}
	}
	oldx = x;
	oldy = y;
	oldscalex = scalex;
	oldscaley = scaley;
}
