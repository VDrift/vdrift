#ifndef _WIDGET_STRINGWHEEL_H
#define _WIDGET_STRINGWHEEL_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"
#include "guioption.h"
#include "scenegraph.h"
#include "font.h"

#include <string>
#include <cassert>
#include <list>
#include <map>

class TEXTURE_GL;

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
	
	void SyncOption()
	{
		if (option)
		{
			label.ReviseDrawable(option->GetCurrentDisplayValue());
			for (std::list <WIDGET *>::iterator n = hooks.begin(); n != hooks.end(); n++)
				(*n)->HookMessage(option->GetCurrentStorageValue());
		}
	}
	
public:
	WIDGET_STRINGWHEEL() : option(NULL) {}
	virtual WIDGET * clone() const {return new WIDGET_STRINGWHEEL(*this);};
	
	void SetupDrawable(SCENENODE * scene, const std::string & newtitle, TEXTURE_GL * teximage_left_up, TEXTURE_GL * teximage_left_down, 
			   TEXTURE_GL * teximage_right_up, TEXTURE_GL * teximage_right_down,
			   FONT * font, float scalex, float scaley, float centerx, float centery)
	{
		assert(scene);
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
	
	virtual void SetAlpha(float newalpha)
	{
		title.SetAlpha(newalpha);
		label.SetAlpha(newalpha);
		button_left.SetAlpha(newalpha);
		button_right.SetAlpha(newalpha);
	}
	
	virtual void SetVisible(bool newvis)
	{
		if (newvis)
			SyncOption();
		title.SetVisible(newvis);
		label.SetVisible(newvis);
		button_left.SetVisible(newvis);
		button_right.SetVisible(newvis);
	}
	
	virtual bool ProcessInput(float cursorx, float cursory, bool cursordown, bool cursorjustup)
	{
		active_action.clear();
		
		bool left = button_left.ProcessInput(cursorx, cursory, cursordown, cursorjustup);
		bool right = button_right.ProcessInput(cursorx, cursory, cursordown, cursorjustup);
		
		if (option)
		{
			if (left && cursorjustup)
			{
				option->Decrement();
				SyncOption();
				active_action = action;
			}
			
			if (right && cursorjustup)
			{
				option->Increment();
				SyncOption();
				active_action = action;
			}
		}
		
		/*if (current != values.end())
		{
			if (left && cursorjustup)
			{
				active_action = action;
				
				if (current == values.begin())
					SetCurrent(values.back().first);
				else
				{
					std::list <std::pair<std::string,std::string> >::iterator i = current;
					i--;
					SetCurrent(i->first);
				}
			}
			
			if (right && cursorjustup)
			{
				active_action = action;
				
				std::list <std::pair<std::string,std::string> >::iterator i = current;
				i++;
				if (i == values.end())
					SetCurrent(values.front().first);
				else
					SetCurrent(i->first);
			}
		}*/
		
		return left || right;
	}
	
	void SetAction(const std::string & newaction)
	{
		action = newaction;
	}
	
	virtual std::string GetAction() const {return active_action;}
	
	/*void SetCurrent(const std::string newsetting)
	{
		current = values.end();
		for (std::list <std::pair<std::string,std::string> >::iterator i = values.begin(); i != values.end(); i++)
		{
			if (i->first == newsetting)
			{
				current = i;
				for (list <WIDGET *>::iterator n = hooks.begin(); n != hooks.end(); n++)
					(*n)->HookMessage(newsetting);
			}
		}
		
		assert (current != values.end());

		label.ReviseDrawable(current->second);
	}*/
	
	/*void SetCurrent(const std::string newsetting, std::ostream & error_output)
	{
		current = values.end();
		for (std::list <std::pair<std::string,std::string> >::iterator i = values.begin(); i != values.end(); i++)
		{
			if (i->first == newsetting)
			{
				current = i;
				for (list <WIDGET *>::iterator n = hooks.begin(); n != hooks.end(); n++)
					(*n)->HookMessage(newsetting);
			}
		}
		
		if (current == values.end())
		{
			error_output << "Option " << setting << " doesn't have value " << newsetting << std::endl;
			label.ReviseDrawable("");
		}
		else
		{
			label.ReviseDrawable(current->second);
		}
	}*/
	
	void SetSetting(const std::string & newsetting)
	{
		setting = newsetting;
	}
	
	//virtual std::string GetAction() const {return active_action;}
	virtual std::string GetDescription() const {return description;}
	virtual void SetDescription(const std::string & newdesc) {description = newdesc;}
	
	///set the local option pointer to the associated optionmap
	virtual void UpdateOptions(bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		option = &(optionmap[setting]);
		
		if (!save_to_options)
			SyncOption();
		
		//error_output << "Updating options: " << save_to_options << std::endl;
		/*if (save_to_options)
		{
			//error_output << "!!!setting option " << setting << " to " << current->first << std::endl;
			if (current == values.end())
				return;
			optionmap[setting].SetCurrentValue(current->first);
		}
		else
		{
			std::string currentsetting = optionmap[setting].GetCurrentStorageValue();
			if (currentsetting.empty())
			{
				//error_output << "Option " << setting << " doesn't have a current value or the current value is blank." << std::endl;
				if (values.empty())
					error_output << "Option " << setting << " also doesn't have any possible values." << std::endl;
				else
					SetCurrent(values.begin()->first, error_output);
			}
			else
			{
				SetCurrent(currentsetting, error_output);
			}
		}*/
	}
	
	virtual void AddHook(WIDGET * other) {hooks.push_back(other);}
	virtual void HookMessage(const std::string & message)
	{
		if (option)
			option->SetToFirstValue();
		
		SyncOption();
		
		//SetCurrent(values.begin()->first);
		
		/*for (list <WIDGET *>::iterator i = hooks.begin(); i != hooks.end(); i++)
			(*i)->HookMessage(message);*/
	}
};

#endif
