#include "widget_label.h"

#include <cassert>

WIDGET_LABEL::WIDGET_LABEL() :
	savedfont(0), r(1), g(1), b(1)
{
	// ctor
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
	const FONT & font,
	const std::string & text,
	float x, float y,
	float scalex, float scaley,
	float nr, float ng, float nb,
	float z, bool centered)
{
	savedfont = &font;
	r = nr;
	b = nb;
	g = ng;
	saved_x = x;
	saved_y = y;
	saved_scalex = scalex;
	saved_scaley = scaley;
	saved_centered = centered;

	draw = scene.GetDrawlist().text.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetDrawOrder(z);

	if (centered) x = x - savedfont->GetWidth(text) * saved_scalex * 0.5;
	text_draw.Set(drawref, font, text, x, y, scalex, scaley, r, g, b);
}

void WIDGET_LABEL::ReviseDrawable(SCENENODE & scene, const std::string & text)
{
	assert(savedfont);

	text_draw.Revise(*savedfont, text);
}

float WIDGET_LABEL::GetWidth(const FONT & font, const std::string & text, float scale) const
{
	return font.GetWidth(text) * scale;
}

void WIDGET_LABEL::SetText(SCENENODE & scene, const std::string & text)
{
	assert(savedfont);

	float x = saved_x;
	if (saved_centered) x = x - savedfont->GetWidth(text) * saved_scalex * 0.5;
	text_draw.Revise(*savedfont, text, x, saved_y, saved_scalex, saved_scaley);
}

const std::string & WIDGET_LABEL::GetText()
{
	return text_draw.GetText();
}
