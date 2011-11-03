#ifndef _TEXT_DRAW_H
#define _TEXT_DRAW_H

#include "font.h"
#include "scenenode.h"
#include "vertexarray.h"

#include <string>
#include <cassert>

class TEXT_DRAW
{
public:
	TEXT_DRAW();

	void Set(
		DRAWABLE & draw,
		const FONT & font, const std::string & newtext,
		float x,  float y, float newscalex, float newscaley,
		float r, float g, float b);

	void Revise(
		const FONT & font, const std::string & newtext,
		float x, float y, float scalex, float scaley);

	void Revise(const FONT & font, const std::string & newtext);

	const std::string & GetText() const
	{
		return text;
	}

	const std::pair<float,float> GetScale() const
	{
		return std::pair<float,float>(oldscalex, oldscaley);
	}

	static float RenderCharacter(
		const FONT & font, char c,
		float x, float y, float scalex, float scaley,
		VERTEXARRAY & output_array);

	static float RenderText(
		const FONT & font, const std::string & newtext,
		float x, float y, float scalex, float scaley,
		VERTEXARRAY & output_array);

	static void SetText(
		DRAWABLE & draw,
		const FONT & font, const std::string & text,
		float x, float y, float scalex, float scaley,
		float r, float g, float b,
		VERTEXARRAY & output_array);

private:
	VERTEXARRAY varray;
	std::string text;
	float oldx, oldy, oldscalex, oldscaley;
};

///a slightly higher level class than the TEXT_DRAW Class that contains its own DRAWABLE handle
class TEXT_DRAWABLE
{
public:
	TEXT_DRAWABLE() : font(NULL),curx(0),cury(0),cr(1),cg(1),cb(1),ca(1) {}

	///this function will add a drawable to parentnode and store the result
	void Init(SCENENODE & parentnode, const FONT & newfont, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley)
	{
		assert(font == NULL);

		draw = parentnode.GetDrawlist().text.insert(DRAWABLE());
		DRAWABLE & drawref = GetDrawable(parentnode);
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

	void SetColor(SCENENODE & parentnode, const float r, const float g, const float b)
	{
		DRAWABLE & drawref = GetDrawable(parentnode);
		cr = r;
		cg = g;
		cb = b;
		drawref.SetColor(cr,cg,cb,ca);
	}

	void SetAlpha(SCENENODE & parentnode, const float a)
	{
		DRAWABLE & drawref = GetDrawable(parentnode);
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

	void SetDrawOrder(SCENENODE & parentnode, float newdo)
	{
		DRAWABLE & drawref = GetDrawable(parentnode);
		drawref.SetDrawOrder(newdo);
	}

	void SetDrawEnable(SCENENODE & parentnode, bool newvis)
	{
		DRAWABLE & drawref = GetDrawable(parentnode);
		drawref.SetDrawEnable(newvis);
	}

	void ToggleDrawEnable(SCENENODE & parentnode)
	{
		DRAWABLE & drawref = GetDrawable(parentnode);
		drawref.SetDrawEnable(!drawref.GetDrawEnable());
	}

	DRAWABLE & GetDrawable(SCENENODE & parentnode)
	{
		return parentnode.GetDrawlist().text.get(draw);
	}

private:
	TEXT_DRAW text;
	keyed_container <DRAWABLE>::handle draw;
	const FONT * font;
	float curx, cury;
	float cr,cg,cb,ca;
};

#endif
