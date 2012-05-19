#include "gui/guislider.h"
#include "gui/guioption.h"

GUISLIDER::GUISLIDER() :
	m_min(0),
	m_max(1),
	m_current(0),
	m_w(0),
	m_h(0),
	m_percent(false),
	m_fill(false)
{
	set_value.call.bind<GUISLIDER, &GUISLIDER::SetValue>(this);
}

GUISLIDER::~GUISLIDER()
{
	// dtor
}

void GUISLIDER::SetAlpha(SCENENODE & scene, float value)
{
	float slider_alpha = value > 0.6 ? value * 0.5 : 0.0;
	m_label_value.SetAlpha(scene, value);
	m_label_left.SetAlpha(scene, value);
	m_label_right.SetAlpha(scene, value);
	m_slider.SetAlpha(scene, slider_alpha);
	m_bar.SetAlpha(scene, slider_alpha * 0.5);
}

void GUISLIDER::SetVisible(SCENENODE & scene, bool value)
{
	m_label_value.SetVisible(scene, value);
	m_label_left.SetVisible(scene, value);
	m_label_right.SetVisible(scene, value);
	m_slider.SetVisible(scene, value);
	m_bar.SetVisible(scene, value);
}

void GUISLIDER::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		float dx = (m_xmax - m_xmin - m_w) * 0.5;
		float dy = (m_ymax - m_ymin - m_h);
		float x = m_xmin + dx;
		float y = m_ymin + dy;
		float w = m_w * (m_current - m_min) / (m_max - m_min);
		float h = m_h - dy;
		if (!m_fill)
		{
			x = x + w - m_h * 0.1;
			w = m_h * 0.2;
		}
		m_slider.SetToBillboard(x, y, w, h);

		float value = m_current;
		std::stringstream s;
		if (m_percent)
		{
			s.precision(0);
			value *= 100.0f;
		}
		else
		{
			s.precision(2);
		}

		s << std::fixed << value;
		if (m_percent)
		{
			s << "%";
		}
		m_label_value.SetText(s.str());

		m_update = false;
	}
}

bool GUISLIDER::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	if (!InFocus(cursorx, cursory))
		return false;

	if (cursordown)
	{
		float xmin = (m_xmin + m_xmax - m_w) * 0.5;
		float coeff = (cursorx - xmin) / m_w;
		coeff = (coeff > 0.0) ? (coeff < 1.0) ? coeff : 1.0 : 0.0;
		m_current = coeff * (m_max - m_min) + m_min;

		std::stringstream s;
		s << m_current;
		signal_value(s.str());
	}

	return true;
}

void GUISLIDER::SetColor(SCENENODE & scene, float r, float g, float b)
{
	m_r = r;
	m_g = g;
	m_b = b;
	m_bar.SetColor(scene, r, g, b);
}

void GUISLIDER::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	std::tr1::shared_ptr<TEXTURE> bartex,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & setting,
	const FONT & font,
	float scalex, float scaley,
	float centerx, float centery,
	float w, float h, float z,
	float min, float max,
	bool percent, bool fill,
	std::ostream & error_output)
{
	assert(bgtex);
	assert(bartex);

	m_setting = setting;
	m_current = 0.0;
	m_min = min;
	m_max = max;
	m_percent = percent;
	m_fill = fill;
	m_xmin = centerx - w * 0.5;
	m_xmax = centerx + w * 0.5;
	m_ymin = centery - h * 0.5;
	m_ymax = centery + h * 0.5;
	float dx = font.GetWidth(" <") * scalex;
	float dy = m_ymax - m_ymin - scaley;
	m_w = w - 2 * dx;
	m_h = scaley;

	m_bar.Load(scene, bgtex, z, error_output);
	m_bar.SetToBillboard(m_xmin + dx, m_ymin + dy, m_w, m_h - dy);
	m_bar.SetColor(scene, 0.7, 0.7, 0.7);

	m_label_value.SetupDrawable(
		scene, font, 0, scalex, scaley,
		centerx, centery, w, h, z + 2,
		m_r, m_g, m_b);

	m_label_left.SetupDrawable(
		scene, font, -1, scalex, scaley,
		centerx - w * 0.25, centery, w * 0.5, h, z + 2,
		m_r, m_g, m_b);

	m_label_right.SetupDrawable(
		scene, font, 1, scalex, scaley,
		centerx + w * 0.25, centery, w * 0.5, h, z + 2,
		m_r, m_g, m_b);

	m_label_left.SetText(" <");
	m_label_right.SetText("> ");

	m_slider.Load(scene, bartex, z + 1, error_output);

	// connect slots, signals
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		i->second.set_val.connect(signal_value);
		set_value.connect(i->second.signal_val);
		SetValue(i->second.GetCurrentStorageValue());
	}
	Update(scene, 0);
}

void GUISLIDER::SetValue(const std::string & valuestr)
{
	float value;
	std::stringstream s(valuestr);
	s >> value;
	if (value != m_current)
	{
		m_current = value;
		m_update = true;
	}
}
