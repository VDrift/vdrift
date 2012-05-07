#include "widget_slider.h"
#include "guioption.h"

WIDGET_SLIDER::WIDGET_SLIDER() :
	m_min(0),
	m_max(1),
	m_current(0),
	m_w(0),
	m_h(0),
	m_percentage(false),
	m_focus(false)
{
	set_value.call.bind<WIDGET_SLIDER, &WIDGET_SLIDER::SetValue>(this);
}

WIDGET_SLIDER::~WIDGET_SLIDER()
{
	// dtor
}

void WIDGET_SLIDER::SetAlpha(SCENENODE & scene, float value)
{
	m_label_value.SetAlpha(scene, value);
	m_label_left.SetAlpha(scene, value);
	m_label_right.SetAlpha(scene, value);
	m_background.SetAlpha(scene, value * 0.25);
	m_bar.SetAlpha(scene, value * 0.5);
}

void WIDGET_SLIDER::SetVisible(SCENENODE & scene, bool value)
{
	m_label_value.SetVisible(scene, value);
	m_label_left.SetVisible(scene, m_focus && value);
	m_label_right.SetVisible(scene, m_focus && value);
	m_background.SetVisible(scene, m_focus && value);
	m_bar.SetVisible(scene, m_focus && value);
}

bool WIDGET_SLIDER::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	m_focus = InFocus(cursorx, cursory);
	m_bar.SetVisible(scene, m_focus);
	m_label_left.SetVisible(scene, m_focus);
	m_label_right.SetVisible(scene, m_focus);
	m_background.SetVisible(scene, m_focus);

	if (!m_focus)
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

std::string WIDGET_SLIDER::GetDescription() const
{
	return m_description;
}

void WIDGET_SLIDER::SetDescription(const std::string & value)
{
	m_description = value;
}

void WIDGET_SLIDER::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		float dx = ((m_xmax - m_xmin) - m_w) * 0.5;
		float xmin = m_xmin + dx;
		float ymin = m_ymin + m_h * 0.15;
		float h = m_h * 0.7;
		float w = m_w * (m_current - m_min) / (m_max - m_min);
		m_bar.SetToBillboard(xmin, ymin, w, h);

		float value = m_current;
		std::stringstream s;
		if (m_percentage)
		{
			s.precision(0);
			value *= 100.0f;
		}
		else
		{
			s.precision(2);
		}

		s << std::fixed << value;
		if (m_percentage)
		{
			s << "%";
		}
		m_label_value.SetText(scene, s.str());

		m_update = false;
	}
}

void WIDGET_SLIDER::SetColor(SCENENODE & scene, float r, float g, float b)
{
	//m_wedge.SetColor(scene, r, g, b);
}

void WIDGET_SLIDER::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	std::tr1::shared_ptr<TEXTURE> bartex,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & setting,
	const FONT & font,
	float scalex, float scaley,
	float centerx, float centery,
	float w, float h, float z,
	float min, float max, bool percentage,
	std::ostream & error_output)
{
	assert(bgtex);
	assert(bartex);

	m_setting = setting;
	m_current = 0.0;
	m_min = min;
	m_max = max;
	m_percentage = percentage;
	m_xmin = centerx - w * 0.5;
	m_xmax = centerx + w * 0.5;
	m_ymin = centery - h * 0.5;
	m_ymax = centery + h * 0.5;
	m_w = w - 2 * font.GetWidth(" <") * scalex;
	m_h = h;

	m_background.Load(scene, bgtex, z, error_output);
	m_background.SetToBillboard(m_xmin, m_ymin, w, h);
	m_background.SetVisible(scene, m_focus);
	m_background.SetColor(scene, 0.7, 0.7, 0.7);
	m_background.SetAlpha(scene, 0.25);

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

	m_label_left.SetText(scene, " <");
	m_label_right.SetText(scene, "> ");

	m_bar.Load(scene, bartex, z + 1, error_output);

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

void WIDGET_SLIDER::SetValue(const std::string & valuestr)
{
	float value;
	std::stringstream s;
	s << valuestr;
	s >> value;
	if (value != m_current)
	{
		m_current = value;
		m_update = true;
	}
}
