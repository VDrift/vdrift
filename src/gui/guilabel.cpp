#include "widget_label.h"

#include <cassert>

WIDGET_LABEL::WIDGET_LABEL() :
	m_font(0),
	m_x(0),
	m_y(0),
	m_scalex(0),
	m_scaley(0),
	m_align(0)
{
	// constructor
}

WIDGET_LABEL::~WIDGET_LABEL()
{
	// destructor
}

void WIDGET_LABEL::SetAlpha(SCENENODE & scene, float value)
{
	GetDrawable(scene).SetColor(m_r, m_g, m_b, value);
}

void WIDGET_LABEL::SetVisible(SCENENODE & scene, bool value)
{
	GetDrawable(scene).SetDrawEnable(value);
}

void WIDGET_LABEL::SetupDrawable(
	SCENENODE & scene,
	const FONT & font,
	int align,
	float scalex, float scaley,
	float x, float y,
	float w, float h, float z,
	float r, float g, float b)
{
	m_font = &font;
	m_r = r;
	m_b = b;
	m_g = g;
	m_x = x;
	m_y = y;
	m_scalex = scalex;
	m_scaley = scaley;
	m_align = align;
	m_xmin = x - w * 0.5;
	m_xmax = x + w * 0.5;
	m_ymin = y - h * 0.5;
	m_ymax = y + h * 0.5;

	m_draw = scene.GetDrawlist().text.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetDrawOrder(z);

	float textw = 0;
	if (align == -1) x = m_xmin ;
	else if (align == 0) x = x - textw * 0.5;
	else if (align == 1) x = m_xmax - textw;
	m_text_draw.Set(drawref, font, m_text, x, y, scalex, scaley, r, g, b);
}

void WIDGET_LABEL::ReviseDrawable(SCENENODE & scene, const std::string & text)
{
	assert(m_font);
	m_text = text;
	m_text_draw.Revise(*m_font, m_text);
}

void WIDGET_LABEL::SetText(SCENENODE & scene, const std::string & text)
{
	assert(m_font);
	m_text = text;
	float x = m_x;
	float textw = m_font->GetWidth(m_text) * m_scalex;
	if (m_align == -1) x = m_xmin;
	else if (m_align == 0) x = x - textw * 0.5;
	else if (m_align == 1) x = m_xmax - textw;
	m_text_draw.Revise(*m_font, m_text, x, m_y, m_scalex, m_scaley);
}

const std::string & WIDGET_LABEL::GetText() const
{
	return m_text;
}

DRAWABLE & WIDGET_LABEL::GetDrawable(SCENENODE & scene)
{
	return scene.GetDrawlist().text.get(m_draw);
}
