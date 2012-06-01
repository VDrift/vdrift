#include "gui/guiwidget.h"
#include "hsvtorgb.h"
#include "drawable.h"
/*
inline static float EaseInOutQuad(float t)
{
	return t < 0.5f ? 2.0f * t * t : -2.0f * (t - 1.0f) * (t - 1.0f) + 1.0f;
}

inline static float EaseInOutQuadInv(float t)
{
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	return t < 0.5f ? sqrt(0.5f * t) : 1.0f - sqrt(0.5f - 0.5f * t);
}

// v, vtarget in [0, 1], dt > 0
inline static float EaseInOut(float v, float vtarget, float dt)
{
	float t = EaseInOutQuadInv(v);
	float v1 = v;
	if (v > vtarget)
	{
		v1 = EaseInOutQuad(t - dt);
		if (v1 < vtarget) v1 = vtarget;
	}
	else if (v < vtarget)
	{
		v1 = EaseInOutQuad(t + dt);
		if (v1 > vtarget) v1 = vtarget;
	}
	return v1;
}

const float GUIWIDGET::m_ease_factor = 4;
*/
GUIWIDGET::GUIWIDGET() :
	//m_r1(1), m_g1(1), m_b1(1), m_a1(1),
	m_r(1), m_g(1), m_b(1), m_a(1),
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
/*	dt = m_ease_factor * dt;
	bool update = false;
	if (fabs(m_r - m_r1) > 1E-4)
	{
		m_r = EaseInOut(m_r, m_r1, dt);
		update = true;
	}
	if (fabs(m_g - m_g1) > 1E-4)
	{
		m_g = EaseInOut(m_g, m_g1, dt);
		update = true;
	}
	if (fabs(m_b - m_b1) > 1E-4)
	{
		m_b = EaseInOut(m_b, m_b1, dt);
		update = true;
	}
	if (fabs(m_a - m_a1) > 1E-4)
	{
		m_a = EaseInOut(m_a, m_a1, dt);
		m_visible = m_a > 0.0f;
		update = true;
	}*/
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
	m_update = true;
}

void GUIWIDGET::SetAlpha(float value)
{
	m_a = (value > 0) ? (value < 1) ? value : 1 : 0;
	m_update = true;
}

void GUIWIDGET::SetHue(float value)
{
	value = (value > 0) ? (value < 1) ? value : 1 : 0;
	
	float h, s, v;
	RGBtoHSV(m_r, m_g, m_b, h, s, v);
	HSVtoRGB(value, s, v, m_r, m_g, m_b);
	m_update = true;
}

void GUIWIDGET::SetSat(float value)
{
	value = (value > 0) ? (value < 1) ? value : 1 : 0;
	
	float h, s, v;
	RGBtoHSV(m_r, m_g, m_b, h, s, v);
	HSVtoRGB(h, value, v, m_r, m_g, m_b);
	m_update = true;
}

void GUIWIDGET::SetVal(float value)
{
	value = (value > 0) ? (value < 1) ? value : 1 : 0;
	
	float h, s, v;
	RGBtoHSV(m_r, m_g, m_b, h, s, v);
	HSVtoRGB(h, s, value, m_r, m_g, m_b);
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
