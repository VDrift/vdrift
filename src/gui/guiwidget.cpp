#include "gui/guiwidget.h"
#include "hsvtorgb.h"
#include "drawable.h"

GUIWIDGET::GUIWIDGET() :
	m_r(1), m_g(1), m_b(1), m_a(1),
	m_h(0), m_s(0), m_v(1),
	m_visible(true),
	m_update(false)
{
	set_color.call.bind<GUIWIDGET, &GUIWIDGET::SetColor>(this);
	set_alpha.call.bind<GUIWIDGET, &GUIWIDGET::SetAlpha>(this);
	set_hue.call.bind<GUIWIDGET, &GUIWIDGET::SetHue>(this);
	set_sat.call.bind<GUIWIDGET, &GUIWIDGET::SetSat>(this);
	set_val.call.bind<GUIWIDGET, &GUIWIDGET::SetVal>(this);
}

GUIWIDGET::~GUIWIDGET()
{
	// destructor
}

void GUIWIDGET::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a);
		GetDrawable(scene).SetDrawEnable(m_visible);
		m_update = false;
	}
}

void GUIWIDGET::SetAlpha(SCENENODE & scene, float value)
{
	GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a * value);
}

void GUIWIDGET::SetVisible(SCENENODE & scene, bool value)
{
	GetDrawable(scene).SetDrawEnable(m_visible & value);
}

void GUIWIDGET::SetColor(float r, float g, float b)
{
	m_r = r, m_g = g, m_b = b;
	RGBtoHSV(m_r, m_g, m_b, m_h, m_s, m_v);
	m_update = true;
}

void GUIWIDGET::SetAlpha(float value)
{
	m_a = (value > 0) ? (value < 1) ? value : 1 : 0;
	m_update = true;
}

void GUIWIDGET::SetHue(float value)
{
	m_h = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GUIWIDGET::SetSat(float value)
{
	m_s = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GUIWIDGET::SetVal(float value)
{
	m_v = (value > 0) ? (value < 1) ? value : 1 : 0;
	HSVtoRGB(m_h, m_s, m_v, m_r, m_g, m_b);
	m_update = true;
}

void GUIWIDGET::SetColor(const std::string & value)
{
	if (value.empty()) return;

	std::stringstream s(value);
	MATHVECTOR<float, 3> v;
	s >> v;
	SetColor(v[0], v[1], v[2]);
}

void GUIWIDGET::SetAlpha(const std::string & value)
{
	if (value.empty()) return;

	std::stringstream s(value);
	float v;
	s >> v;
	SetAlpha(v);
}

void GUIWIDGET::SetHue(const std::string & value)
{
	if (value.empty()) return;

	std::stringstream s(value);
	float v;
	s >> v;
	SetHue(v);
}

void GUIWIDGET::SetSat(const std::string & value)
{
	if (value.empty()) return;

	std::stringstream s(value);
	float v;
	s >> v;
	SetSat(v);
}

void GUIWIDGET::SetVal(const std::string & value)
{
	if (value.empty()) return;

	std::stringstream s(value);
	float v;
	s >> v;
	SetVal(v);
}
