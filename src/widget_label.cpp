#include "widget_label.h"

#include <cassert>

WIDGET_LABEL::WIDGET_LABEL() : 
	savedfont(0),
	r(1), g(1), b(1)
{
	// ctor
}

WIDGET * WIDGET_LABEL::clone() const
{
	return new WIDGET_LABEL(*this);
}


void WIDGET_LABEL::SetAlpha(SCENENODE & scene, float newalpha)
{
	GetDrawable(scene).SetColor(r, g, b, newalpha);
}

void WIDGET_LABEL::SetVisible(SCENENODE & scene, bool newvis)
{
	GetDrawable(scene).SetDrawEnable(newvis);
}

void WIDGET_LABEL::SetupDrawable(
	SCENENODE & scene,
	const FONT * font,
	const std::string & text,
	float x, float y,
	float scalex, float scaley,
	float nr, float ng, float nb,
	int order,
	bool centered)
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

	draw = scene.GetDrawlist().text.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	text_draw.Set(drawref, *font, text, x-w*0.5, y, scalex, scaley, r,g,b);
	drawref.SetDrawOrder(order+100);
}

void WIDGET_LABEL::ReviseDrawable(
	SCENENODE & scene,
	const FONT * font,
	const std::string & text,
	float x, float y,
	float scalex, float scaley,
	bool centered)
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

void WIDGET_LABEL::ReviseDrawable(SCENENODE & scene, const std::string & text)
{
	assert(savedfont);
	
	//float w = text_draw.GetWidth(*savedfont, text, scale);
	
	text_draw.Revise(*savedfont, text);
}

float WIDGET_LABEL::GetWidth(const FONT * font, const std::string & text, float scale) const
{
	assert(font);
	//assert(!text.empty());
	return text_draw.GetWidth(*font, text, scale);
}

void WIDGET_LABEL::SetText(SCENENODE & scene, const std::string & text)
{
	assert(savedfont);
	
	float w = text_draw.GetWidth(*savedfont, text, saved_scalex);
	if (!saved_centered)
		w = 0;
	
	text_draw.Revise(*savedfont, text, saved_x-w*0.5, saved_y, saved_scalex, saved_scaley);
}

const std::string & WIDGET_LABEL::GetText()
{
	return text_draw.GetText();
}