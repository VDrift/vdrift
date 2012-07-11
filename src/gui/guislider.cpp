#include "gui/guislider.h"

GUISLIDER::GUISLIDER() :
	m_value(0), m_x(0), m_y(0), m_w(0), m_h(0), m_fill(false)
{
	set_value.call.bind<GUISLIDER, &GUISLIDER::SetValue>(this);
}

GUISLIDER::~GUISLIDER()
{
	// dtor
}

void GUISLIDER::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		float x = m_x;
		float w = m_w * m_value;
		if (!m_fill)
		{
			x = x + w - m_h * 0.1;
			w = m_h * 0.2;
		}
		m_slider.SetToBillboard(x, m_y, w, m_h);
		
		GUIWIDGET::Update(scene, dt);
	}
}

void GUISLIDER::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> texture,
	float centerx, float centery,
	float w, float h, float z, bool fill,
	std::ostream & error_output)
{
	m_value = 0;
	m_x = centerx - w * 0.5;
	m_y = centery - h * 0.5;
	m_w = w;
	m_h = h;
	m_fill = fill;
	m_slider.Load(scene, texture, z);
}

void GUISLIDER::SetValue(const std::string & valuestr)
{
	float value;
	std::stringstream s(valuestr);
	s >> value;
	if (value != m_value)
	{
		m_value = value;
		m_update = true;
	}
}

DRAWABLE & GUISLIDER::GetDrawable(SCENENODE & scene)
{
	return m_slider.GetDrawable(scene);
}
