#include "widget_doublestringwheel.h"
#include "guioption.h"

#include <cassert>

WIDGET_DOUBLESTRINGWHEEL::WIDGET_DOUBLESTRINGWHEEL() :
	update(false)
{
	set_value1.call.bind<WIDGET_DOUBLESTRINGWHEEL, &WIDGET_DOUBLESTRINGWHEEL::SetValue1>(this);
	set_value2.call.bind<WIDGET_DOUBLESTRINGWHEEL, &WIDGET_DOUBLESTRINGWHEEL::SetValue2>(this);
}

WIDGET_DOUBLESTRINGWHEEL::~WIDGET_DOUBLESTRINGWHEEL()
{
	// dtor
}

void WIDGET_DOUBLESTRINGWHEEL::SetAlpha(SCENENODE & scene, float newalpha)
{
	title.SetAlpha(scene, newalpha);
	label.SetAlpha(scene, newalpha);
	button_left.SetAlpha(scene, newalpha);
	button_right.SetAlpha(scene, newalpha);
}

void WIDGET_DOUBLESTRINGWHEEL::SetVisible(SCENENODE & scene, bool newvis)
{
	title.SetVisible(scene, newvis);
	label.SetVisible(scene, newvis);
	button_left.SetVisible(scene, newvis);
	button_right.SetVisible(scene, newvis);
}

std::string WIDGET_DOUBLESTRINGWHEEL::GetDescription() const
{
	return description;
}

void WIDGET_DOUBLESTRINGWHEEL::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

void WIDGET_DOUBLESTRINGWHEEL::Update(SCENENODE & scene, float dt)
{
	if (update)
	{
		std::string value = value1 + "," + value2;
		label.ReviseDrawable(scene, value);
		update = false;
	}
}

bool WIDGET_DOUBLESTRINGWHEEL::ProcessInput(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	bool left = button_left.ProcessInput(scene, optionmap, cursorx, cursory, cursordown, cursorjustup);
	bool right = button_right.ProcessInput(scene, optionmap, cursorx, cursory, cursordown, cursorjustup);

	if (left && cursorjustup)
	{
		optionmap[setting1].Decrement();
		optionmap[setting2].Decrement();
	}
	else if (right && cursorjustup)
	{
		optionmap[setting1].Increment();
		optionmap[setting2].Increment();
	}

	return left || right;
}

void WIDGET_DOUBLESTRINGWHEEL::SetupDrawable(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & newsetting1,
	const std::string & newsetting2,
	const std::string & newtitle,
	std::tr1::shared_ptr<TEXTURE> left_up,
	std::tr1::shared_ptr<TEXTURE> left_down,
	std::tr1::shared_ptr<TEXTURE> right_up,
	std::tr1::shared_ptr<TEXTURE> right_down,
	const FONT & font,
	float scalex,
	float scaley,
	float centerx,
	float centery,
	float z)
{
	assert(left_up);
	assert(left_down);
	assert(right_up);
	assert(right_down);

	float titlewidth = title.GetWidth(font, newtitle, scalex);
	float blx = centerx + titlewidth + scalex / 2;
	float brx = blx + scalex * 3 / 4;
	float labelx = brx + scalex / 2;
	float r(1), g(1), b(1);
	float h(0), w(0);
	title.SetupDrawable(scene, font, newtitle, centerx, centery, scalex, scaley, r, g, b, z, false);
	label.SetupDrawable(scene, font, "", labelx, centery, scalex, scaley, r, g, b, z, false);
	button_left.SetupDrawable(scene, left_up, left_down, left_up, font, "", blx, centery, scalex, scaley, r, g, b, h, w, z);
	button_right.SetupDrawable(scene, right_up, right_down, right_up, font, "", brx, centery, scalex, scaley, r, g, b, h, w, z);

	setting1 = newsetting1;
	setting2 = newsetting2;
	std::map<std::string, GUIOPTION>::iterator i;
	i = optionmap.find(setting1);
	if (i != optionmap.end())
	{
		set_value1.connect(i->second.signal_str);
		SetValue1(i->second.GetCurrentDisplayValue());
	}
	i = optionmap.find(setting2);
	if (i != optionmap.end())
	{
		set_value2.connect(i->second.signal_str);
		SetValue2(i->second.GetCurrentDisplayValue());
	}
	Update(scene, 0);
}

void WIDGET_DOUBLESTRINGWHEEL::SetValue1(const std::string & valuenew)
{
	if (value1 != valuenew)
	{
		value1 = valuenew;
		update = true;
	}
}

void WIDGET_DOUBLESTRINGWHEEL::SetValue2(const std::string & valuenew)
{
	if (value2 != valuenew)
	{
		value2 = valuenew;
		update = true;
	}
}
