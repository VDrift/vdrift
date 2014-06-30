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

#ifndef _TEXT_DRAW_H
#define _TEXT_DRAW_H

#include "font.h"
#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"

#include <string>
#include <cassert>

class TextDraw
{
public:
	TextDraw();

	void Set(
		Drawable & draw,
		const Font & font, const std::string & newtext,
		float x,  float y, float newscalex, float newscaley,
		float r, float g, float b);

	void Revise(
		const Font & font, const std::string & newtext,
		float x, float y, float scalex, float scaley);

	void Revise(const Font & font, const std::string & newtext);

	const std::string & GetText() const
	{
		return text;
	}

	const std::pair<float,float> GetScale() const
	{
		return std::pair<float,float>(oldscalex, oldscaley);
	}

	static float RenderCharacter(
		const Font & font, char c,
		float x, float y, float scalex, float scaley,
		VertexArray & output_array);

	static float RenderText(
		const Font & font, const std::string & newtext,
		float x, float y, float scalex, float scaley,
		VertexArray & output_array);

	static void SetText(
		Drawable & draw,
		const Font & font, const std::string & text,
		float x, float y, float scalex, float scaley,
		float r, float g, float b,
		VertexArray & output_array);

private:
	VertexArray varray;
	std::string text;
	float oldx, oldy, oldscalex, oldscaley;
};

///a slightly higher level class than the TEXT_DRAW Class that contains its own DRAWABLE handle
class TextDrawable
{
public:
	TextDrawable() : font(NULL),curx(0),cury(0),cr(1),cg(1),cb(1),ca(1) {}

	///this function will add a drawable to parentnode and store the result
	void Init(SceneNode & parentnode, const Font & newfont, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley)
	{
		assert(font == NULL);

		draw = parentnode.GetDrawList().text.insert(Drawable());
		Drawable & drawref = GetDrawable(parentnode);
		font = &newfont;
		curx = x;
		cury = y;
		text.Set(drawref, *font, newtext, x, y, newscalex, newscaley, cr,cg,cb);
		SetAlpha(parentnode, ca);
	}

	void Revise(const std::string & newtext)
	{
		assert(font);
		text.Revise(*font, newtext, curx, cury, text.GetScale().first, text.GetScale().second);
	}

	void Revise(const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley)
	{
		curx = x;
		cury = y;
		text.Revise(*font, newtext, curx, cury, newscalex, newscaley);
	}

	void SetPosition(float newx, float newy)
	{
		assert(font);
		curx = newx;
		cury = newy;
		text.Revise(*font, text.GetText(), curx, cury, text.GetScale().first, text.GetScale().second);
	}

	void SetColor(SceneNode & parentnode, const float r, const float g, const float b)
	{
		Drawable & drawref = GetDrawable(parentnode);
		cr = r;
		cg = g;
		cb = b;
		drawref.SetColor(cr,cg,cb,ca);
	}

	void SetAlpha(SceneNode & parentnode, const float a)
	{
		Drawable & drawref = GetDrawable(parentnode);
		ca = a;
		drawref.SetColor(cr,cg,cb,ca);
	}

	float GetWidth() const
	{
		assert (font);
		return font->GetWidth(text.GetText()) * text.GetScale().first;
	}

	float GetWidth(const std::string & newstr) const
	{
		assert (font);
		return font->GetWidth(newstr) * text.GetScale().first;
	}

	void SetDrawOrder(SceneNode & parentnode, float newdo)
	{
		Drawable & drawref = GetDrawable(parentnode);
		drawref.SetDrawOrder(newdo);
	}

	void SetDrawEnable(SceneNode & parentnode, bool newvis)
	{
		Drawable & drawref = GetDrawable(parentnode);
		drawref.SetDrawEnable(newvis);
	}

	void ToggleDrawEnable(SceneNode & parentnode)
	{
		Drawable & drawref = GetDrawable(parentnode);
		drawref.SetDrawEnable(!drawref.GetDrawEnable());
	}

	Drawable & GetDrawable(SceneNode & parentnode)
	{
		return parentnode.GetDrawList().text.get(draw);
	}

private:
	TextDraw text;
	SceneNode::DrawableHandle draw;
	const Font * font;
	float curx, cury;
	float cr,cg,cb,ca;
};

#endif
