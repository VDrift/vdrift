#ifndef _WIDGET_DOUBLESTRINGWHEEL_H
#define _WIDGET_DOUBLESTRINGWHEEL_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"
#include "font.h"

#include <string>
#include <cassert>
#include <list>
#include <map>

class SCENENODE;
class TEXTURE;

class WIDGET_DOUBLESTRINGWHEEL : public WIDGET
{
private:
	WIDGET_LABEL title;
	WIDGET_LABEL label;
	WIDGET_BUTTON button_left;
	WIDGET_BUTTON button_right;
	std::list <std::pair<std::string,std::string> > values1, values2;
	std::list <std::pair<std::string,std::string> >::iterator current1, current2;
	std::string description;
	std::string setting1, setting2;
	
public:
	WIDGET_DOUBLESTRINGWHEEL() {current1 = values1.end();current2 = values2.end();}
	
	virtual WIDGET * clone() const {return new WIDGET_DOUBLESTRINGWHEEL(*this);};
	
	void SetupDrawable(
		SCENENODE & scene,
		const std::string & newtitle,
		std::tr1::shared_ptr<TEXTURE> teximage_left_up,
		std::tr1::shared_ptr<TEXTURE> teximage_left_down, 
		std::tr1::shared_ptr<TEXTURE> teximage_right_up,
		std::tr1::shared_ptr<TEXTURE> teximage_right_down,
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
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		title.SetAlpha(scene, newalpha);
		label.SetAlpha(scene, newalpha);
		button_left.SetAlpha(scene, newalpha);
		button_right.SetAlpha(scene, newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		title.SetVisible(scene, newvis);
		label.SetVisible(scene, newvis);
		button_left.SetVisible(scene, newvis);
		button_right.SetVisible(scene, newvis);
	}
	
	virtual bool ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
	{
		bool left = button_left.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);
		bool right = button_right.ProcessInput(scene, cursorx, cursory, cursordown, cursorjustup);
		
		if (current1 != values1.end())
		{
			if (left && cursorjustup)
			{
				if (current1 == values1.begin())
					SetCurrent(scene, values1.back().first, values2.back().first);
				else
				{
					std::list <std::pair<std::string,std::string> >::iterator i1 = current1;
					i1--;
					std::list <std::pair<std::string,std::string> >::iterator i2 = current2;
					i2--;
					SetCurrent(scene, i1->first, i2->first);
				}
			}
			
			if (right && cursorjustup)
			{
				std::list <std::pair<std::string,std::string> >::iterator i1 = current1;
				i1++;
				std::list <std::pair<std::string,std::string> >::iterator i2 = current2;
				i2++;
				if (i1 == values1.end())
					SetCurrent(scene, values1.front().first, values2.front().first);
				else
					SetCurrent(scene, i1->first, i2->first);
			}
		}
		
		return left || right;
	}
	
	void SetCurrent(SCENENODE & scene, const std::string newsetting1, const std::string newsetting2)
	{
		current1 = values1.end();
		current2 = values2.end();
		//int offset = -1;
		//int count = 0;
		//for (std::list <std::pair<std::string,std::string> >::iterator i1 = values1.begin(),std::list <std::pair<std::string,std::string> >::iterator i2 = values2.begin();
		//		   i1 != values1.end(),i2 != values2.end(); i1++,i2++)
		std::list <std::pair<std::string,std::string> >::iterator i1 = values1.begin();
		std::list <std::pair<std::string,std::string> >::iterator i2 = values2.begin();
		while (i1 != values1.end() && i2 != values2.end())
		{
			if (i1->first == newsetting1 && i2->first == newsetting2)
			{
				//offset = count;
				current1 = i1;
				current2 = i2;
			}
			//count++;
			i1++;i2++;
		}
		
		/*count = 0;
		for (std::list <std::pair<std::string,std::string> >::iterator i = values2.begin(); i != values2.end(); i++)
		{
			if (count == offset)
				current2 = i;
			count++;
		}*/
		
		assert (current1 != values1.end());
		assert (current2 != values2.end());

		label.ReviseDrawable(scene, current1->second+","+current2->second);
	}
	
	void SetCurrent(SCENENODE & scene, const std::string newsetting1, const std::string newsetting2, std::ostream & error_output)
	{
		current1 = values1.end();
		current2 = values2.end();
		//int offset = -1;
		//int count = 0;
		//for (std::list <std::pair<std::string,std::string> >::iterator i1 = values1.begin(),std::list <std::pair<std::string,std::string> >::iterator i2 = values2.begin();
		//		   i1 != values1.end(),i2 != values2.end(); i1++,i2++)
		std::list <std::pair<std::string,std::string> >::iterator i1 = values1.begin();
		std::list <std::pair<std::string,std::string> >::iterator i2 = values2.begin();
		while (i1 != values1.end() && i2 != values2.end())
		{
			if (i1->first == newsetting1 && i2->first == newsetting2)
			{
				//offset = count;
				current1 = i1;
				current2 = i2;
			}
			//count++;
			i1++;i2++;
		}
		
		if (current1 == values1.end() || current2 == values2.end())
		{
			error_output << "Option " << setting1 << " doesn't have value " << newsetting1 << " or option " << setting2 << " doesn't pair with " << newsetting2 << std::endl;
			label.ReviseDrawable(scene, "");
		}
		else
		{
			label.ReviseDrawable(scene, current1->second+","+current2->second);
		}
	}
	
	void SetValueList(const std::list <std::pair<std::string,std::string> > & newvaluelist1,const std::list <std::pair<std::string,std::string> > & newvaluelist2)
	{
		values1 = newvaluelist1;
		values2 = newvaluelist2;
		assert(values1.size() == values2.size());
	}
	
	void SetSetting(const std::string & newsetting1, const std::string & newsetting2)
	{
		setting1 = newsetting1;
		setting2 = newsetting2;
	}
	
	//virtual std::string GetAction() const {return active_action;}
	virtual std::string GetDescription() const {return description;}
	virtual void SetDescription(const std::string & newdesc) {description = newdesc;}
	virtual void UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
	{
		//error_output << "Updating options: " << save_to_options << std::endl;
		if (save_to_options)
		{
			//error_output << "!!!setting option " << setting << " to " << current->first << std::endl;
			if (current1 == values1.end() || current2 == values2.end())
				return;
			optionmap[setting1].SetCurrentValue(current1->first);
			optionmap[setting2].SetCurrentValue(current2->first);
		}
		else
		{
			std::string currentsetting1 = optionmap[setting1].GetCurrentStorageValue();
			std::string currentsetting2 = optionmap[setting2].GetCurrentStorageValue();
			if (currentsetting1.empty() || currentsetting2.empty())
			{
				error_output << "Option pair " << setting1 << "," << setting2 << " doesn't have a current value or the current value is blank." << std::endl;
				if (values1.empty())
					error_output << "Option " << setting1 << " also doesn't have any possible values." << std::endl;
				else
					SetCurrent(scene, values1.begin()->first, values2.begin()->first, error_output);
			}
			else
			{
				SetCurrent(scene, currentsetting1, currentsetting2, error_output);
			}
		}
	}
};

#endif
