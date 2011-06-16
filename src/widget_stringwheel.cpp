#include "widget_stringwheel.h"
#include "guioption.h"

WIDGET_STRINGWHEEL::WIDGET_STRINGWHEEL() :
	option(0)
{
	// ctor
}

WIDGET * WIDGET_STRINGWHEEL::clone() const
{
	return new WIDGET_STRINGWHEEL(*this);
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
	if (newvis) SyncOption(scene);
	title.SetVisible(scene, newvis);
	label.SetVisible(scene, newvis);
	button_left.SetVisible(scene, newvis);
	button_right.SetVisible(scene, newvis);
}

bool WIDGET_STRINGWHEEL::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
{
	active_action.clear();

	bool left = button_left.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);
	bool right = button_right.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);

	if (option)
	{
		if (left && cursorjustup)
		{
			option->Decrement();
			SyncOption(scene);
			active_action = action;
		}

		if (right && cursorjustup)
		{
			option->Increment();
			SyncOption(scene);
			active_action = action;
		}
	}

	return left || right;
}

void WIDGET_STRINGWHEEL::SetName(const std::string & newname)
{
	name = newname;
}

std::string WIDGET_STRINGWHEEL::GetDescription() const
{
	return description;
}

void WIDGET_STRINGWHEEL::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

///set the local option pointer to the associated optionmap
void WIDGET_STRINGWHEEL::UpdateOptions(
	SCENENODE & scene,
	bool save_to,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	option = &(optionmap[setting]);
	if (!save_to) SyncOption(scene);
}

void WIDGET_STRINGWHEEL::AddHook(WIDGET * other)
{
	hooks.push_back(other);
}

void WIDGET_STRINGWHEEL::HookMessage(SCENENODE & scene, const std::string & message, const std::string & from)
{
	if (option) option->SetToFirstValue();
	SyncOption(scene);
}

void WIDGET_STRINGWHEEL::SetSetting(const std::string & newsetting)
{
	setting = newsetting;
}

void WIDGET_STRINGWHEEL::SetupDrawable(
	SCENENODE & scene,
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
}

void WIDGET_STRINGWHEEL::SyncOption(SCENENODE & scene)
{
	if (option)
	{
		label.ReviseDrawable(scene, option->GetCurrentDisplayValue());
		for (std::list <WIDGET *>::iterator n = hooks.begin(); n != hooks.end(); n++)
		{
			(*n)->HookMessage(scene, option->GetCurrentStorageValue(), name);
		}
	}
}
