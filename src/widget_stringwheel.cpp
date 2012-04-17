#include "widget_stringwheel.h"
#include "guioption.h"

WIDGET_STRINGWHEEL::WIDGET_STRINGWHEEL() :
	update(false)
{
	set_value.call.bind<WIDGET_STRINGWHEEL, &WIDGET_STRINGWHEEL::SetValue>(this);
}

WIDGET_STRINGWHEEL::~WIDGET_STRINGWHEEL()
{
	// dtor
}

void WIDGET_STRINGWHEEL::SetAlpha(SCENENODE & scene, float newalpha)
{
	title.SetAlpha(scene, newalpha);
	label.SetAlpha(scene, newalpha);
	button_left.SetAlpha(scene, newalpha);
	button_right.SetAlpha(scene, newalpha);
}

void WIDGET_STRINGWHEEL::SetVisible(SCENENODE & scene, bool newvis)
{
	title.SetVisible(scene, newvis);
	label.SetVisible(scene, newvis);
	button_left.SetVisible(scene, newvis);
	button_right.SetVisible(scene, newvis);
}

bool WIDGET_STRINGWHEEL::ProcessInput(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	bool left = button_left.ProcessInput(scene, optionmap, cursorx, cursory, cursordown, cursorjustup);
	bool right = button_right.ProcessInput(scene, optionmap, cursorx, cursory, cursordown, cursorjustup);

	active_action.clear();
	if (cursorjustup)
	{
		if (left)
		{
			optionmap[setting].Decrement();
			active_action = action;
		}
		else if (right)
		{
			optionmap[setting].Increment();
			active_action = action;
		}
	}

	return left || right;
}

std::string WIDGET_STRINGWHEEL::GetDescription() const
{
	return description;
}

void WIDGET_STRINGWHEEL::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

void WIDGET_STRINGWHEEL::Update(SCENENODE & scene, float dt)
{
	if (update)
	{
		label.ReviseDrawable(scene, value);
		update = false;
	}
}

void WIDGET_STRINGWHEEL::SetupDrawable(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & settingnew,
	const std::string & titlenew,
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

	float titlewidth = title.GetWidth(font, titlenew, scalex);
	float blx = centerx + titlewidth + scalex / 2;
	float brx = blx + scalex * 3 / 4;
	float labelx = brx + scalex / 2;
	float r(1), g(1), b(1);
	float h(0), w(0);
	title.SetupDrawable(scene, font, titlenew, centerx, centery, scalex, scaley, r, g, b, z, false);
	label.SetupDrawable(scene, font, "", labelx, centery, scalex, scaley, r, g, b, z, false);
	button_left.SetupDrawable(scene, left_up, left_down, left_up, font, "", blx, centery, scalex, scaley, r, g, b, h, w, z);
	button_right.SetupDrawable(scene, right_up, right_down, right_up, font, "", brx, centery, scalex, scaley, r, g, b, h, w, z);

	// connect slot
	setting = settingnew;
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		set_value.connect(i->second.signal_str);
		SetValue(i->second.GetCurrentDisplayValue());
	}
	Update(scene, 0);
}

void WIDGET_STRINGWHEEL::SetValue(const std::string & valuenew)
{
	if (value != valuenew)
	{
		value = valuenew;
		update = true;
	}
}
