#include "gui/guilabel.h"

#include <cassert>

GUILABEL::GUILABEL() :
	m_font(0),
	m_x(0),
	m_y(0),
	m_scalex(0),
	m_scaley(0),
	m_align(0)
{
	set_value.call.bind<GUILABEL, &GUILABEL::SetText>(this);
}

GUILABEL::~GUILABEL()
{
	// destructor
}

void GUILABEL::SetupDrawable(
	SCENENODE & scene,
	const FONT & font,
	int align,
	float scalex, float scaley,
	float x, float y,
	float w, float h, float z)
{
	m_font = &font;
	m_x = x;
	m_y = y;
	m_w = w;
	m_h = h;
	m_scalex = scalex;
	m_scaley = scaley;
	m_align = align;

	m_draw = scene.GetDrawlist().text.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetDrawOrder(z);

	float textw = 0;
	if (align == -1) x -= w * 0.5;
	else if (align == 0) x -= textw * 0.5;
	else if (align == 1) x -= (textw - w * 0.5);
	m_text_draw.Set(drawref, font, m_text, x, y, scalex, scaley, m_r, m_g, m_b);
}

void GUILABEL::SetText(const std::string & text)
{
	assert(m_font);
	m_text = text;
	float x = m_x;
	float textw = m_font->GetWidth(m_text) * m_scalex;
	if (m_align == -1) x -= m_w * 0.5;
	else if (m_align == 0) x -= textw * 0.5;
	else if (m_align == 1) x -= (textw - m_w * 0.5);
	m_text_draw.Revise(*m_font, m_text, x, m_y, m_scalex, m_scaley);
}

const std::string & GUILABEL::GetText() const
{
	return m_text;
}

DRAWABLE & GUILABEL::GetDrawable(SCENENODE & scene)
{
	return scene.GetDrawlist().text.get(m_draw);
}
