#include "gui/guistringwheel2.h"
#include "gui/guioption.h"

#include <cassert>

GUISTRINGWHEEL2::GUISTRINGWHEEL2() :
	m_focus(false)
{
	set_value1.call.bind<GUISTRINGWHEEL2, &GUISTRINGWHEEL2::SetValue1>(this);
	set_value2.call.bind<GUISTRINGWHEEL2, &GUISTRINGWHEEL2::SetValue2>(this);
}

GUISTRINGWHEEL2::~GUISTRINGWHEEL2()
{
	// dtor
}

void GUISTRINGWHEEL2::SetAlpha(SCENENODE & scene, float value)
{
	m_label_value.SetAlpha(scene, value);
	m_label_left.SetAlpha(scene, value);
	m_label_right.SetAlpha(scene, value);
}

void GUISTRINGWHEEL2::SetVisible(SCENENODE & scene, bool value)
{
	m_label_value.SetVisible(scene, value);
	m_label_left.SetVisible(scene, m_focus && value);
	m_label_right.SetVisible(scene, m_focus && value);
}

std::string GUISTRINGWHEEL2::GetDescription() const
{
	return m_description;
}

void GUISTRINGWHEEL2::SetDescription(const std::string & value)
{
	m_description = value;
}

void GUISTRINGWHEEL2::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		std::string value = m_value1 + "x" + m_value2;
		m_label_value.SetText(scene, value);
		m_update = false;
	}
}

bool GUISTRINGWHEEL2::ProcessInput(
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
			prev_value1();
			prev_value2();
		}
		else if (focus_right)
		{
			next_value1();
			next_value2();
		}
	}

	return m_focus;
}

void GUISTRINGWHEEL2::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & setting1,
	const std::string & setting2,
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

	m_setting1 = setting1;
	m_setting2 = setting2;
	std::map<std::string, GUIOPTION>::iterator i;
	i = optionmap.find(m_setting1);
	if (i != optionmap.end())
	{
		i->second.prev_val.connect(prev_value1);
		i->second.next_val.connect(next_value1);
		set_value1.connect(i->second.signal_str);
		SetValue1(i->second.GetCurrentDisplayValue());
	}
	i = optionmap.find(m_setting2);
	if (i != optionmap.end())
	{
		i->second.prev_val.connect(prev_value2);
		i->second.next_val.connect(next_value2);
		set_value2.connect(i->second.signal_str);
		SetValue2(i->second.GetCurrentDisplayValue());
	}
	Update(scene, 0);
}

void GUISTRINGWHEEL2::SetValue1(const std::string & value)
{
	if (m_value1 != value)
	{
		m_value1 = value;
		m_update = true;
	}
}

void GUISTRINGWHEEL2::SetValue2(const std::string & value)
{
	if (m_value2 != value)
	{
		m_value2 = value;
		m_update = true;
	}
}
