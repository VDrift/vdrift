#ifndef _TEXT_DRAW_H
#define _TEXT_DRAW_H

#include <string>
#include <cassert>

#include "font.h"
#include "scenegraph.h"
#include "vertexarray.h"

class TEXT_DRAW
{
private:
	VERTEXARRAY varray;
	std::string text;
	float oldx, oldy, oldscalex, oldscaley;
	
	float RenderCharacter(VERTEXARRAY & output_array, const float tw, const float th, const float x, const float y, const float scalex, const float scaley, const FONT::CHARINFO & c);
	
public:
	TEXT_DRAW() : oldx(0), oldy(0), oldscalex(1.0), oldscaley(1.0) {}
	
	void Set(DRAWABLE & draw, const FONT & font, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley, const float r=1, const float g=1, const float b=1)
	{
		Set(draw, font, newtext, x, y, newscalex, newscaley, r,g,b,varray);
	}
	
	void Set(DRAWABLE & draw, const FONT & font, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley, const float r, const float g, const float b, VERTEXARRAY & output_array);
	
	void Revise(DRAWABLE & draw, const FONT & font, const std::string & newtext)
	{
		Revise(draw, font, newtext, oldx, oldy, oldscalex, oldscaley);
	}
	
	void Revise(DRAWABLE & draw, const FONT & font, const std::string & newtext, float x, float y, float scalex, float scaley)
	{
		Revise(draw, font, newtext, x, y, scalex, scaley, varray);
	}
	
	void Revise(DRAWABLE & draw, const FONT & font, const std::string & newtext, float x, float y, float scalex, float scaley, VERTEXARRAY & output_array);
	
	float GetWidth(const FONT & font, const std::string & newtext, const float newscale) const;

	const std::string & GetText() const
	{
		return text;
	}
	
	const std::pair<float,float> GetCurrentScale() const {return std::pair<float,float>(oldscalex,oldscaley);}
};

///a slightly higher level class than the TEXT_DRAW Class that contains its own DRAWABLE pointer
class TEXT_DRAWABLE
{
private:
	TEXT_DRAW text;
	DRAWABLE * draw;
	const FONT * font;
	float curx, cury;
	float cr,cg,cb,ca;
	
public:
	TEXT_DRAWABLE() : draw(NULL),font(NULL),curx(0),cury(0),cr(1),cg(1),cb(1),ca(1) {}
	
	///this function will call parentnode.AddDrawable() and store the result
	void Init(SCENENODE & parentnode, const FONT & newfont, const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley)
	{
		assert(draw == NULL);
		assert (font == NULL);
		
		draw = &parentnode.AddDrawable();
		assert (draw);
		font = &newfont;
		curx = x;
		cury = y;
		text.Set(*draw, *font, newtext, x, y, newscalex, newscaley, cr,cg,cb);
		SetAlpha(ca);
	}
	
	void Revise(const std::string & newtext)
	{
		assert(draw);
		assert(font);
		text.Revise(*draw, *font, newtext, curx, cury, text.GetCurrentScale().first, text.GetCurrentScale().second);
	}
	
	void Revise(const std::string & newtext, const float x, const float y, const float newscalex, const float newscaley)
	{
		curx = x;
		cury = y;
		text.Revise(*draw, *font, newtext, curx, cury, newscalex, newscaley);
	}
	
	void SetPosition(float newx, float newy)
	{
		assert(draw);
		assert(font);
		curx = newx;
		cury = newy;
		text.Revise(*draw, *font, text.GetText(), curx, cury, text.GetCurrentScale().first, text.GetCurrentScale().second);
	}
	
	void SetColor(const float r, const float g, const float b)
	{
		assert(draw);
		cr = r;
		cg = g;
		cb = b;
		draw->SetColor(cr,cg,cb,ca);
	}
	
	void SetAlpha(const float a)
	{
		ca = a;
		draw->SetColor(cr,cg,cb,ca);
	}
	
	float GetWidth() const
	{
		assert (font);
		return text.GetWidth(*font, text.GetText(), text.GetCurrentScale().first);
	}
	
	void SetDrawOrder(float newdo)
	{
		assert(draw);
		draw->SetDrawOrder(newdo);
	}
	
	void SetDrawEnable(bool newvis)
	{
		assert(draw);
		draw->SetDrawEnable(newvis);
	}
	
	void ToggleDrawEnable()
	{
		assert(draw);
		draw->SetDrawEnable(!draw->GetDrawEnable());
	}
	
	DRAWABLE * GetDrawable() {return draw;}
};

#endif
