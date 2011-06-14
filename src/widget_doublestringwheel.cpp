#include "widget_doublestringwheel.h"
#include "guioption.h"

#include <cassert>

WIDGET_DOUBLESTRINGWHEEL::WIDGET_DOUBLESTRINGWHEEL()
{
	current1 = values1.end();
	current2 = values2.end();
}

WIDGET * WIDGET_DOUBLESTRINGWHEEL::clone() const
{
	return new WIDGET_DOUBLESTRINGWHEEL(*this);
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

void WIDGET_DOUBLESTRINGWHEEL::UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output)
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

bool WIDGET_DOUBLESTRINGWHEEL::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
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

void WIDGET_DOUBLESTRINGWHEEL::SetupDrawable(
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

void WIDGET_DOUBLESTRINGWHEEL::SetCurrent(SCENENODE & scene, const std::string & newsetting1, const std::string & newsetting2)
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

void WIDGET_DOUBLESTRINGWHEEL::SetCurrent(SCENENODE & scene, const std::string & newsetting1, const std::string & newsetting2, std::ostream & error_output)
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

void WIDGET_DOUBLESTRINGWHEEL::SetValueList(const std::list <std::pair<std::string,std::string> > & newvaluelist1,const std::list <std::pair<std::string,std::string> > & newvaluelist2)
{
	values1 = newvaluelist1;
	values2 = newvaluelist2;
	assert(values1.size() == values2.size());
}

void WIDGET_DOUBLESTRINGWHEEL::SetSetting(const std::string & newsetting1, const std::string & newsetting2)
{
	setting1 = newsetting1;
	setting2 = newsetting2;
}

//virtual std::string GetAction() const {return active_action;}
std::string WIDGET_DOUBLESTRINGWHEEL::GetDescription() const
{
	return description;
}

void WIDGET_DOUBLESTRINGWHEEL::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}
