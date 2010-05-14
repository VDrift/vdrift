#include "widget_slider.h"

#include "guioption.h"

WIDGET * WIDGET_SLIDER::clone() const
{
	return new WIDGET_SLIDER(*this);
}

void WIDGET_SLIDER::SetAlpha(SCENENODE & scene, float newalpha)
{
	wedge.SetAlpha(scene, newalpha);
	cursor.SetAlpha(scene, newalpha);
	text.SetAlpha(scene, newalpha);
}

void WIDGET_SLIDER::SetVisible(SCENENODE & scene, bool newvis)
{
	wedge.SetVisible(scene, newvis);
	cursor.SetVisible(scene, newvis);
	text.SetDrawEnable(scene, newvis);
}

bool WIDGET_SLIDER::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
{
	if (cursorx-(corner2[0]-corner1[0])*0.25 < corner2[0] && cursorx+(corner2[0]-corner1[0])*0.25 > corner1[0] &&
		cursory < corner2[1] && cursory > corner1[1])
	{
		if (cursordown)
		{
			float coeff = (cursorx-corner1[0])/(corner2[0]-corner1[0]);
			if (coeff < 0)
				coeff = 0;
			if (coeff > 1.0)
				coeff = 1.0;
			//std::cout << coeff << std::endl;
			current = coeff*(max-min)+min;
			cursor.SetToBillboard(corner1[0]+coeff*(corner2[0]-corner1[0]), corner1[1], w, h);
			UpdateText(scene);
			std::stringstream s;
			s << current;
			SendMessage(scene, s.str());
		}
		
		return true;
	}
	
	return false;
}

void WIDGET_SLIDER::SetName(const std::string & newname)
{
	name = newname;
}

std::string WIDGET_SLIDER::GetDescription() const
{
	return description;
}

void WIDGET_SLIDER::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

void WIDGET_SLIDER::UpdateOptions(
	SCENENODE & scene,
	bool save_to_options,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	if (setting.empty())
		return;
	
	if (save_to_options)
	{
		std::stringstream s;
		s << current;
		optionmap[setting].SetCurrentValue(s.str());
		//std::cout << "Saving option value to: " << current << " | " << s.str() << std::endl;
	}
	else
	{
		std::string currentsetting = optionmap[setting].GetCurrentStorageValue();
		if (currentsetting.empty())
		{
			//error_output << "Option " << setting << " doesn't have a current value or the current value is blank; defaulting to 0.0." << std::endl;
			//current = 0.0;
			//UpdateText();
		}
		else
		{
			std::stringstream s(currentsetting);
			s >> current;
			float coeff = (current-min)/(max-min);
			cursor.SetToBillboard(corner1[0]+coeff*(corner2[0]-corner1[0]), corner1[1], w, h);
			UpdateText(scene);
			SendMessage(scene, currentsetting);
			//std::cout << "Loading option value for " << setting << ": " << current << " | " << currentsetting << std::endl;
		}
	}
}

void WIDGET_SLIDER::AddHook(WIDGET * other)
{
	hooks.push_back(other);
}

void WIDGET_SLIDER::SetColor(SCENENODE & scene, float r, float g, float b)
{
	wedge.SetColor(scene, r, g, b);
}

void WIDGET_SLIDER::SetSetting(const std::string & newsetting)
{
	setting = newsetting;
}

void WIDGET_SLIDER::SetupDrawable(
	SCENENODE & scene,
	TEXTUREPTR wedgetex,
	TEXTUREPTR cursortex, 
	float x, float y,
	float nw, float nh,
	float newmin, float newmax,
	bool ispercentage,
	const std::string & newsetting,
	FONT * font,
	float fontscalex,
	float fontscaley,
	std::ostream & error_output,
	int draworder)
{
	assert(wedgetex);
	assert(cursortex);
	assert(font);
	
	current = 0.0;
	percentage = ispercentage;
	min = newmin;
	max = newmax;
	w = nw;
	h = nh;
	setting = newsetting;
	
	wedge.Load(scene, wedgetex, draworder, error_output);
	cursor.Load(scene, cursortex, draworder+1, error_output);
	
	corner1.Set(x-w*4.0*0.5, y-h*0.5);
	corner2.Set(x+w*4.0*0.5*0.5, y+h*0.5);
	
	text.Init(scene, *font, "", corner1[0], (corner1[1]+corner2[1])*0.5, fontscalex, fontscaley);
	UpdateText(scene);
	text.SetDrawOrder(scene, draworder);
	
	wedge.SetToBillboard(corner1[0], corner1[1], w*4.0, h*4.0);
	//cursor.SetToBillboard(x, y-h*0.375, w*0.25, h*0.25);
	cursor.SetToBillboard(corner1[0], corner1[1], w, h);
}

void WIDGET_SLIDER::UpdateText(SCENENODE & scene)
{
	std::stringstream s;
	if (percentage)
		s.precision(0);
	else
		s.precision(2);
	float value = current;
	if (percentage)
		value *= 100.0f;
	s << std::fixed << value;
	if (percentage)
		s << "%";
	text.Revise(s.str());
	float width = text.GetWidth();
	float newx = corner1[0] - width - w*0.25;
	float newy = (corner1[1]+corner2[1])*0.5;
	text.SetPosition(newx, newy);
}

void WIDGET_SLIDER::SendMessage(SCENENODE & scene, const std::string message) const
{
	for (std::list <WIDGET *>::const_iterator n = hooks.begin(); n != hooks.end(); n++)
	{
		(*n)->HookMessage(scene, message, name);
	}
}
