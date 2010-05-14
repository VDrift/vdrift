#include "widget_stringwheel.h"

#include "guioption.h"

WIDGET_STRINGWHEEL::WIDGET_STRINGWHEEL()
: option(NULL)
{

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
	if (newvis)
		SyncOption(scene);
	title.SetVisible(scene, newvis);
	label.SetVisible(scene, newvis);
	button_left.SetVisible(scene, newvis);
	button_right.SetVisible(scene, newvis);
}

bool WIDGET_STRINGWHEEL::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
{
	bool left = button_left.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);
	bool right = button_right.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);
	
	if (option)
	{
		if (left && cursorjustup)
		{
			option->Decrement();
			SyncOption(scene);
		}
		
		if (right && cursorjustup)
		{
			option->Increment();
			SyncOption(scene);
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
void WIDGET_STRINGWHEEL::UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
{
	option = &(optionmap[setting]);
	
	if (!save_to_options)
		SyncOption(scene);
}

void WIDGET_STRINGWHEEL::AddHook(WIDGET * other)
{
	hooks.push_back(other);
}

void WIDGET_STRINGWHEEL::HookMessage(SCENENODE & scene, const std::string & message, const std::string & from)
{
	if (option)
		option->SetToFirstValue();
	
	SyncOption(scene);
}

void WIDGET_STRINGWHEEL::SetSetting(const std::string & newsetting)
{
	setting = newsetting;
}

void WIDGET_STRINGWHEEL::SetupDrawable(
	SCENENODE & scene,
	const std::string & newtitle,
	TEXTUREPTR teximage_left_up,
	TEXTUREPTR teximage_left_down, 
	TEXTUREPTR teximage_right_up,
	TEXTUREPTR teximage_right_down,
	FONT * font,
	float scalex,
	float scaley,
	float centerx,
	float centery)
{
	assert(teximage_left_up);
	assert(teximage_left_down);
	assert(teximage_right_up);
	assert(teximage_right_down);
	assert(font);
	
	float titlewidth = title.GetWidth(font, newtitle, scalex);
	float labeloffsetx = 0.04+titlewidth;
	float labeloffsety = scaley*0.05;
	
	float buttonsize = 0.125;
	
	//setup drawable functions for sub-widgets
	title.SetupDrawable(scene, font, newtitle, centerx, centery+labeloffsety, scalex,scaley, 1,1,1, 1, false);
	label.SetupDrawable(scene, font, "", centerx+labeloffsetx+0.02, centery+labeloffsety, scalex,scaley, 1,1,1, 1, false);
	button_left.SetupDrawable(scene, teximage_left_up, teximage_left_down, teximage_left_up, font, "", centerx+0.02+titlewidth, centery, buttonsize,buttonsize, 1,1,1);
	button_right.SetupDrawable(scene, teximage_right_up, teximage_right_down, teximage_right_up, font, "", centerx+0.02*2.0+titlewidth, centery, buttonsize,buttonsize, 1,1,1);
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
