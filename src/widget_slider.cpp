#include "widget_slider.h"
#include "guioption.h"

WIDGET_SLIDER::WIDGET_SLIDER() :
	min(0),
	max(1),
	current(0),
	update(false)
{
	set_value.call.bind<WIDGET_SLIDER, &WIDGET_SLIDER::SetValue>(this);
}

WIDGET_SLIDER::~WIDGET_SLIDER()
{
	// dtor
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

bool WIDGET_SLIDER::ProcessInput(
	SCENENODE & scene,
	std::map<std::string, GUIOPTION> & optionmap,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	if (cursorx < corner2[0] + w * 0.5 &&
		cursorx > corner1[0] - w * 0.5 &&
		cursory < corner2[1] &&
		cursory > corner1[1])
	{
		if (cursordown)
		{
			float coeff = (cursorx - corner1[0]) / (corner2[0] - corner1[0]);
			if (coeff < 0) coeff = 0;
			else if (coeff > 1.0) coeff = 1.0;
			current = coeff * (max - min) + min;

			std::stringstream s;
			s << current;
			optionmap[setting].SetCurrentValue(s.str());
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

void WIDGET_SLIDER::Update(SCENENODE & scene, float dt)
{
	if (update)
	{
		float dx = (corner2[0] - corner1[0]) * (current - min) / (max - min);
		cursor.SetToBillboard(corner1[0] + dx, corner1[1], w, h);

		float value = current;

		std::stringstream s;
		if (percentage)
		{
			s.precision(0);
			value *= 100.0f;
		}
		else
		{
			s.precision(2);
		}

		s << std::fixed << value;
		if (percentage)
		{
			s << "%";
		}

		text.Revise(s.str());

		float width = text.GetWidth();
		float newx = corner1[0] - width - w * 0.25;
		text.SetPosition(newx, texty);

		update = false;
	}
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
	std::tr1::shared_ptr<TEXTURE> wedgetex,
	std::tr1::shared_ptr<TEXTURE> cursortex,
	const float x,
	const float y,
	const float nw,
	const float nh,
	const float newmin,
	const float newmax,
	const bool ispercentage,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & newsetting,
	const FONT & font,
	const float scalex,
	const float scaley,
	std::ostream & error_output,
	int draworder)
{
	assert(wedgetex);
	assert(cursortex);

	current = 0.0;
	percentage = ispercentage;
	min = newmin;
	max = newmax;
	w = nw;
	h = nh;
	setting = newsetting;

	wedge.Load(scene, wedgetex, draworder, error_output);
	cursor.Load(scene, cursortex, draworder + 1, error_output);

	corner1.Set(x - w * 4.0 * 0.5, y - h * 0.5);
	corner2.Set(x + w * 4.0 * 0.5, y + h * 0.5);
	texty = y + (h - scaley) * 0.5;

	text.Init(scene, font, "", corner1[0], texty, scalex, scaley);
	text.SetDrawOrder(scene, draworder);
	wedge.SetTo2DQuad(corner1[0], corner1[1], corner2[0], corner2[1], 0, 0, 13.0/16.0, 1.0/4.0, 0);

	// connect slot
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		set_value.connect(i->second.signal_val);
		SetValue(i->second.GetCurrentStorageValue());
	}
	Update(scene, 0);
}

void WIDGET_SLIDER::SetValue(const std::string & valuestr)
{
	float value;
	std::stringstream s;
	s << valuestr;
	s >> value;
	if (value != current)
	{
		current = value;
		update = true;
	}
}
