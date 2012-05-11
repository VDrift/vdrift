#include "widget_stringwheel.h"
#include "guioption.h"

WIDGET_STRINGWHEEL::WIDGET_STRINGWHEEL() :
	m_focus(false)
{
	set_value.call.bind<WIDGET_STRINGWHEEL, &WIDGET_STRINGWHEEL::SetValue>(this);
}

WIDGET_STRINGWHEEL::~WIDGET_STRINGWHEEL()
{
	// dtor
}

void WIDGET_STRINGWHEEL::SetAlpha(SCENENODE & scene, float value)
{
	m_label_value.SetAlpha(scene, value);
	m_label_left.SetAlpha(scene, value);
	m_label_right.SetAlpha(scene, value);
}

void WIDGET_STRINGWHEEL::SetVisible(SCENENODE & scene, bool value)
{
	m_label_value.SetVisible(scene, value);
	m_label_left.SetVisible(scene, m_focus && value);
	m_label_right.SetVisible(scene, m_focus && value);
}

bool WIDGET_STRINGWHEEL::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	bool focus_left = m_label_left.InFocus(cursorx, cursory);
	bool focus_right = !focus_left && m_label_right.InFocus(cursorx, cursory);

	m_focus = focus_left || focus_right;
	m_label_left.SetVisible(scene, m_focus);
	m_label_right.SetVisible(scene, m_focus);

	if (cursorjustup)
	{
		if (focus_left)
		{
			prev_value();
			m_action_active = m_action;
		}
		else if (focus_right)
		{
			next_value();
			m_action_active = m_action;
		}
	}
	else
	{
		m_action_active.clear();
	}

	return m_focus;
}

std::string WIDGET_STRINGWHEEL::GetDescription() const
{
	return m_description;
}

void WIDGET_STRINGWHEEL::SetDescription(const std::string & value)
{
	m_description = value;
}

void WIDGET_STRINGWHEEL::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		m_label_value.SetText(scene, m_value);
		m_update = false;
	}
}

std::string WIDGET_STRINGWHEEL::GetAction() const
{
	return m_action_active;
}

void WIDGET_STRINGWHEEL::SetAction(const std::string & value)
{
	m_action = value;
}

void WIDGET_STRINGWHEEL::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & setting,
	const FONT & font,
	float scalex, float scaley,
	float centerx, float centery,
	float w, float h, float z,
	std::ostream & error_output)
{
	assert(bgtex);

	m_xmin = centerx - w * 0.5;
	m_xmax = centerx + w * 0.5;
	m_ymin = centery - h * 0.5;
	m_ymax = centery + h * 0.5;

	m_label_value.SetupDrawable(
		scene, font, 0, scalex, scaley,
		centerx, centery, w, h, z + 1,
		m_r, m_g, m_b);

	m_label_left.SetupDrawable(
		scene, font, -1, scalex, scaley,
		centerx - w * 0.25, centery, w * 0.5, h, z + 1,
		m_r, m_g, m_b);

	m_label_right.SetupDrawable(
		scene, font, 1, scalex, scaley,
		centerx + w * 0.25, centery, w * 0.5, h, z + 1,
		m_r, m_g, m_b);

	m_label_left.SetText(scene, " <");
	m_label_right.SetText(scene, "> ");

	// connect slots, signals
	m_setting = setting;
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		i->second.prev_val.connect(prev_value);
		i->second.next_val.connect(next_value);
		set_value.connect(i->second.signal_str);
		SetValue(i->second.GetCurrentDisplayValue());
	}
	Update(scene, 0);
}

void WIDGET_STRINGWHEEL::SetValue(const std::string & value)
{
	if (m_value != value)
	{
		m_value = value;
		m_update = true;
	}
}
