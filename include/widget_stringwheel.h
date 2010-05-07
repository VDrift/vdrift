#ifndef _WIDGET_STRINGWHEEL_H
#define _WIDGET_STRINGWHEEL_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"
#include "guioption.h"
#include "font.h"

#include <string>
#include <cassert>
#include <list>
#include <map>

class SCENENODE;
class TEXTURE;

class WIDGET_STRINGWHEEL : public WIDGET
{
private:
	WIDGET_LABEL title;
	WIDGET_LABEL label;
	WIDGET_BUTTON button_left;
	WIDGET_BUTTON button_right;
	std::string action;
	std::string active_action;
	GUIOPTION * option;
	std::string description;
	std::string setting;
	std::list <WIDGET *> hooks;
	
	void SyncOption(SCENENODE & scene)
	{
		if (option)
		{
			label.ReviseDrawable(scene, option->GetCurrentDisplayValue());
			for (std::list <WIDGET *>::iterator n = hooks.begin(); n != hooks.end(); n++)
				(*n)->HookMessage(scene, option->GetCurrentStorageValue());
		}
	}
	
public:
	WIDGET_STRINGWHEEL() : option(NULL) {}
	virtual WIDGET * clone() const {return new WIDGET_STRINGWHEEL(*this);};
	
	void SetupDrawable(SCENENODE & scene, const std::string & newtitle, TEXTUREPTR teximage_left_up, TEXTUREPTR teximage_left_down, 
			   TEXTUREPTR teximage_right_up, TEXTUREPTR teximage_right_down,
			   FONT * font, float scalex, float scaley, float centerx, float centery)
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
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		title.SetAlpha(scene, newalpha);
		label.SetAlpha(scene, newalpha);
		button_left.SetAlpha(scene, newalpha);
		button_right.SetAlpha(scene, newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		if (newvis)
			SyncOption(scene);
		title.SetVisible(scene, newvis);
		label.SetVisible(scene, newvis);
		button_left.SetVisible(scene, newvis);
		button_right.SetVisible(scene, newvis);
	}
	
	virtual bool ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
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
	
	void SetAction(const std::string & newaction)
	{
		action = newaction;
	}
	
	void SetSetting(const std::string & newsetting)
	{
		setting = newsetting;
	}
	
	//virtual std::string GetAction() const {return active_action;}
	virtual std::string GetDescription() const {return description;}
	virtual void SetDescription(const std::string & newdesc) {description = newdesc;}
	
	///set the local option pointer to the associated optionmap
	virtual void UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		option = &(optionmap[setting]);
		
		if (!save_to_options)
			SyncOption(scene);
	}
	
	virtual void AddHook(WIDGET * other) {hooks.push_back(other);}
	virtual void HookMessage(SCENENODE & scene, const std::string & message)
	{
		if (option)
			option->SetToFirstValue();
		
		SyncOption(scene);
		
		//SetCurrent(values.begin()->first);
		
		/*for (list <WIDGET *>::iterator i = hooks.begin(); i != hooks.end(); i++)
			(*i)->HookMessage(message);*/
	}
};

#endif
