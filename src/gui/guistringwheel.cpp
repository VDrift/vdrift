#include "gui/guistringwheel.h"
#include "gui/guioption.h"

GUISTRINGWHEEL::GUISTRINGWHEEL()
{
	set_value.call.bind<GUISTRINGWHEEL, &GUISTRINGWHEEL::SetValue>(this);
}

GUISTRINGWHEEL::~GUISTRINGWHEEL()
{
	// dtor
}

void GUISTRINGWHEEL::SetAlpha(SCENENODE & scene, float value)
{
	m_label_value.SetAlpha(scene, value);
	m_label_left.SetAlpha(scene, value);
	m_label_right.SetAlpha(scene, value);
}

void GUISTRINGWHEEL::SetVisible(SCENENODE & scene, bool value)
{
	m_label_value.SetVisible(scene, value);
	m_label_left.SetVisible(scene, value);
	m_label_right.SetVisible(scene, value);
}

void GUISTRINGWHEEL::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		m_label_value.SetText(m_value);
		m_update = false;
	}
}

bool GUISTRINGWHEEL::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	bool focus_left = m_label_left.InFocus(cursorx, cursory);
	bool focus_right = !focus_left && m_label_right.InFocus(cursorx, cursory);

	if (cursorjustup)
	{
		if (focus_left)
		{
			OnMoveLeft();
		}
		else if (focus_right)
		{
			OnMoveRight();
		}
	}

	return focus_left || focus_right;
}

void GUISTRINGWHEEL::SetupDrawable(
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

	m_label_left.SetText(" <");
	m_label_right.SetText("> ");

	// connect slots, signals
	m_setting = setting;
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		set_value.connect(i->second.signal_str);
		SetValue(i->second.GetCurrentDisplayValue());
	}
}

void GUISTRINGWHEEL::SetValue(const std::string & value)
{
	if (m_value != value)
	{
		m_value = value;
		m_update = true;
	}
}
