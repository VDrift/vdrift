#include "widget_button.h"

#include <cassert>

WIDGET_BUTTON::WIDGET_BUTTON() : cancel(true)
{
	// ctor
}

WIDGET * WIDGET_BUTTON::clone() const
{
	return new WIDGET_BUTTON(*this);
}

void WIDGET_BUTTON::SetAlpha(SCENENODE & scene, float newalpha)
{
	label.SetAlpha(scene, newalpha);
	image_up.SetAlpha(scene, newalpha);
	image_down.SetAlpha(scene, newalpha);
	image_selected.SetAlpha(scene, newalpha);
}

void WIDGET_BUTTON::SetVisible(SCENENODE & scene, bool newvis)
{
	label.SetVisible(scene, newvis);
	if (state == UP)
		image_up.SetVisible(scene, newvis);
	else if (state == DOWN)
		image_down.SetVisible(scene, newvis);
	else if (state == SELECTED)
		image_selected.SetVisible(scene, newvis);
}

std::string WIDGET_BUTTON::GetAction() const
{
	return active_action;
}

std::string WIDGET_BUTTON::GetDescription() const
{
	return description;
}

void WIDGET_BUTTON::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

bool WIDGET_BUTTON::GetCancel() const
{
	return cancel;
}

bool WIDGET_BUTTON::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
{
	active_action.clear();
	
	if (cursorx < image_up.GetCorner2()[0] &&
		cursorx > image_up.GetCorner1()[0] &&
		cursory < image_up.GetCorner2()[1] &&
		cursory > image_up.GetCorner1()[1])
	{
		if (cursordown && state != DOWN)
		{
			state = DOWN;
			image_down.SetVisible(scene, true);
			image_up.SetVisible(scene, false);
			image_selected.SetVisible(scene, false);
			
			//std::cout << "depress" << std::endl;
		}
		else if (!cursordown && state != SELECTED)
		{
			state = SELECTED;
			image_down.SetVisible(scene, false);
			image_up.SetVisible(scene, false);
			image_selected.SetVisible(scene, true);
		}
		
		//std::cout << "hover" << std::endl << std::endl;
		
		if (cursorjustup)
		{
			//take some action
			active_action = action;
		}
		
		return true;
	}
	else
	{
		if (state != UP)
		{
			state = UP;
			image_down.SetVisible(scene, false);
			image_up.SetVisible(scene, true);
			image_selected.SetVisible(scene, false);
		}
		
		//std::cout << image_up.GetCorner1() << " x " << image_up.GetCorner2() << cursorx << "," << cursory << std::endl << std::endl;
		return false;
	}
}

void WIDGET_BUTTON::SetCancel(bool newcancel)
{
	cancel = newcancel;
}

void WIDGET_BUTTON::SetAction(const std::string & newaction)
{
	action = newaction;
}

void WIDGET_BUTTON::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> teximage_up,
	std::tr1::shared_ptr<TEXTURE> teximage_down,
	std::tr1::shared_ptr<TEXTURE> teximage_selected,
	const FONT & font,
	const std::string & text,
	float centerx, float centery,
	float scalex, float scaley,
	float r, float g, float b,
	float h, float w)
{
	assert(teximage_up);
	assert(teximage_down);
	assert(teximage_selected);
	
	// enlarge button to fit the text contained
	if (h < scaley) h = scaley;
	if (w < font.GetWidth(text) * scalex) w = font.GetWidth(text) * scalex;
	float hwratio = scaley / scalex;
	
	label.SetupDrawable(scene, font, text, centerx, centery, scalex, scaley, r, g, b, 2);
	image_up.SetupDrawable(scene, teximage_up, centerx, centery, w, h, 1, true, hwratio);
	image_down.SetupDrawable(scene, teximage_down, centerx, centery, w, h, 1, true, hwratio);
	image_selected.SetupDrawable(scene, teximage_selected, centerx, centery, w, h, 1, true, hwratio);
	image_down.SetVisible(scene, false);
	image_selected.SetVisible(scene, false);
	state = UP;
}