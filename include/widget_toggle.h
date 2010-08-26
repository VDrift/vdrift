#ifndef _WIDGET_TOGGLE_H
#define _WIDGET_TOGGLE_H

#include "widget.h"
#include "widget_image.h"
#include "mathvector.h"
#include "guioption.h"

#include <string>
#include <cassert>
#include <iostream>
#include <map>
#include <iostream>

class SCENENODE;
class TEXTURE;

class WIDGET_TOGGLE : public WIDGET
{
public:
	enum TOGGLESTATE
	{
		UP,
		DOWN,
		UPSEL,
		DOWNSEL,
		UPTRANS,
		DOWNTRANS
	};
	
private:
	WIDGET_IMAGE image_up;
	WIDGET_IMAGE image_down;
	WIDGET_IMAGE image_upsel;
	WIDGET_IMAGE image_downsel;
	WIDGET_IMAGE image_transition;
	std::string description;
	TOGGLESTATE state;
	std::string setting;
	bool wasvisible;
	
public:
	WIDGET_TOGGLE() : wasvisible(false) {}
	
	virtual WIDGET * clone() const {return new WIDGET_TOGGLE(*this);};
	
	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> teximage_up,
		std::tr1::shared_ptr<TEXTURE> teximage_down, 
		std::tr1::shared_ptr<TEXTURE> teximage_upselected,
		std::tr1::shared_ptr<TEXTURE> teximage_downselected, 
		std::tr1::shared_ptr<TEXTURE> teximage_transition,
		float centerx,
		float centery,
		float w,
		float h)
	{
		assert(teximage_up);
		assert(teximage_down);
		assert(teximage_upselected);
		assert(teximage_downselected);
		assert(teximage_transition);
		
		image_up.SetupDrawable(scene, teximage_up, centerx, centery, w, h, 1);
		image_down.SetupDrawable(scene, teximage_down, centerx, centery, w, h, 1);
		image_upsel.SetupDrawable(scene, teximage_upselected, centerx, centery, w, h, 1);
		image_downsel.SetupDrawable(scene, teximage_downselected, centerx, centery, w, h, 1);
		image_transition.SetupDrawable(scene, teximage_transition, centerx, centery, w, h, 1);
		
		SetState(scene, UP);
	}
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		//if (newalpha > 0 && !wasvisible) std::cout << "Changing alpha on invisible toggle: " << newalpha << std::endl;
		
		image_up.SetAlpha(scene, newalpha);
		image_down.SetAlpha(scene, newalpha);
		image_upsel.SetAlpha(scene, newalpha);
		image_downsel.SetAlpha(scene, newalpha);
		image_transition.SetAlpha(scene, newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		//if (newvis != wasvisible) std::cout << this << " New vis: " << newvis << ", " << wasvisible << " " << &wasvisible << std::endl;
		
		wasvisible = newvis;
		
		image_down.SetVisible(scene, false);
		image_up.SetVisible(scene, false);
		image_upsel.SetVisible(scene, false);
		image_downsel.SetVisible(scene, false);
		image_transition.SetVisible(scene, false);
		
		if (state == UP)
			image_up.SetVisible(scene, newvis);
		else if (state == DOWN)
			image_down.SetVisible(scene, newvis);
		else if (state == UPSEL)
			image_upsel.SetVisible(scene, newvis);
		else if (state == DOWNSEL)
			image_downsel.SetVisible(scene, newvis);
		else if (state == UPTRANS || state == DOWNTRANS)
			image_transition.SetVisible(scene, newvis);
	}
	
	virtual bool ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
	{
		if (cursorx < image_up.GetCorner2()[0] && cursorx > image_up.GetCorner1()[0] &&
			cursory < image_up.GetCorner2()[1] && cursory > image_up.GetCorner1()[1])
		{
			if (!cursordown)
			{
				if (state == DOWN || state == DOWNTRANS)
				{
					SetState(scene, DOWNSEL);
				}
				else if (state == UP || state == UPTRANS)
				{
					SetState(scene, UPSEL);
				}
			}
			else
			{
				if (state == DOWN || state == DOWNSEL)
				{
					SetState(scene, UPTRANS);
				}
				else if (state == UP || state == UPSEL)
				{
					SetState(scene, DOWNTRANS);
				}
			}
			
			return true;
		}
		else
		{
			if (state == UPSEL || state == UPTRANS)
			{
				SetState(scene, UP);
			}
			else if (state == DOWNSEL || state == DOWNTRANS)
			{
				SetState(scene, DOWN);
			}
			
			//std::cout << image_up.GetCorner1() << " x " << image_up.GetCorner2() << cursorx << "," << cursory << std::endl << std::endl;
			return false;
		}
	}
	
	void SetState(SCENENODE & scene, const TOGGLESTATE & newstate)
	{
		state = newstate;
		
		//std::cout << "State set to: " << (int) state << ", " << wasvisible << std::endl;
		
		//std::cout << "Was visible: " << wasvisible << std::endl;
		
		//if (wasvisible) std::cout << "Refreshing state visibility" << std::endl;
		
		SetVisible(scene, wasvisible);
	}
	
	void SetSetting(const std::string & newsetting) {setting = newsetting;}
	
	virtual std::string GetDescription() const {return description;}
	
	virtual void SetDescription(const std::string & newdesc) {description = newdesc;}
	
	virtual void UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		if (setting.empty())
			return;
		
		if (save_to_options)
		{
			bool current_value = false;
			if (state == DOWN || state == DOWNSEL || state == DOWNTRANS)
				current_value = true;
			optionmap[setting].SetCurrentValue(current_value ? "true" : "false");
		}
		else
		{
			std::string currentsetting = optionmap[setting].GetCurrentStorageValue();
			if (currentsetting.empty())
			{
				//error_output << "Option " << setting << " doesn't have a current value or the current value is blank; defaulting to false." << std::endl;
						//assert(!currentsetting.empty());
						//return false;
				currentsetting = "false";
			}
			else if (currentsetting == "true")
			{
				SetState(scene, DOWN);
			}
			else if (currentsetting == "false")
			{
				SetState(scene, UP);
			}
			else
			{
				error_output << "Option " << setting << " has an unexpected value for a bool: " << currentsetting << std::endl;
			}
		}
	}
};

#endif
