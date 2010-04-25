#ifndef _WIDGET_LABEL_H
#define _WIDGET_LABEL_H

#include "widget.h"
#include "mathvector.h"
#include "scenegraph.h"
#include "vertexarray.h"
#include "text_draw.h"
#include "font.h"

#include <string>
#include <cassert>

class TEXTURE_GL;

class WIDGET_LABEL : public WIDGET
{
private:
	TEXT_DRAW text_draw;
	keyed_container <DRAWABLE>::handle draw;
	FONT * savedfont;
	float r,g,b;
	float saved_x, saved_y, saved_scalex, saved_scaley;
	bool saved_centered;
	
	DRAWABLE & GetDrawable(SCENENODE & scene)
	{
		return scene.GetDrawlist().twodim.get(draw);
	}
	
public:
	WIDGET_LABEL() : savedfont(NULL),r(1),g(1),b(1) {}
	virtual WIDGET * clone() const {return new WIDGET_LABEL(*this);};
	
	void SetupDrawable(SCENENODE & scene, FONT * font, const std::string & text, float x, float y, float scalex, float scaley, const float nr, const float ng, const float nb, int order=0, bool centered=true)
	{
		assert(font);
		savedfont = font;
		
		r = nr;
		b = nb;
		g = ng;
		
		saved_x = x;
		saved_y = y;
		saved_scalex = scalex;
		saved_scaley = scaley;
		saved_centered = centered;
		
		float w = text_draw.GetWidth(*font, text, scalex);
		if (!centered)
			w = 0;

		draw = scene.GetDrawlist().twodim.insert(DRAWABLE());
		DRAWABLE & drawref = GetDrawable(scene);
		text_draw.Set(drawref, *font, text, x-w*0.5, y, scalex, scaley, r,g,b);
		drawref.SetDrawOrder(order+100);
	}
	
	void ReviseDrawable(SCENENODE & scene, FONT * font, const std::string & text, float x, float y, float scalex, float scaley, bool centered=true)
	{
		assert(font);
		
		savedfont = font;
		saved_x = x;
		saved_y = y;
		saved_scalex = scalex;
		saved_scaley = scaley;
		saved_centered = centered;
		
		float w = text_draw.GetWidth(*font, text, scalex);
		if (!centered)
			w = 0;
		
		text_draw.Revise(*font, text, x-w*0.5, y, scalex, scaley);
	}
	
	void SetText(SCENENODE & scene, const std::string & text)
	{
		assert(savedfont);
		
		float w = text_draw.GetWidth(*savedfont, text, saved_scalex);
		if (!saved_centered)
			w = 0;
		
		text_draw.Revise(*savedfont, text, saved_x-w*0.5, saved_y, saved_scalex, saved_scaley);
	}
	
	void ReviseDrawable(SCENENODE & scene, const std::string & text)
	{
		assert(savedfont);
		
		//float w = text_draw.GetWidth(*savedfont, text, scale);
		
		text_draw.Revise(*savedfont, text);
	}
	
	float GetWidth(FONT * font, const std::string & text, float scale) const
	{
		assert(font);
		//assert(!text.empty());
		return text_draw.GetWidth(*font, text, scale);
	}
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		GetDrawable(scene).SetColor(r,g,b,newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		GetDrawable(scene).SetDrawEnable(newvis);
	}
	
	const std::string & GetText()
	{
		return text_draw.GetText();
	}
};

#endif
