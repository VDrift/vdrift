#include "widget_colorpicker.h"
#include "guioption.h"
#include "mathvector.h"

static void HSVtoRGB(float h, float s, float v, float & r, float & g, float & b)
{
	if (s == 0)
	{
		r = v;
		g = v;
		b = v;
	}
	else
	{
		float hi = std::floor(h * 6 + 1.0E-5); // add eps to avoid nummerical precision issues
		float f = h * 6 - hi;
		float p = v * (1 - s);
		float q = v * (1 - (s * f));
		float t = v * (1 - (s * (1 - f)));

		if (hi == 1)
		{
			r = q;
			g = v;
			b = p;
		}
		else if (hi == 2)
		{
			r = p;
			g = v;
			b = t;
		}
		else if (hi == 3)
		{
			r = p;
			g = q;
			b = v;
		}
		else if (hi == 4)
		{
			r = t;
			g = p;
			b = v;
		}
		else if (hi == 5)
		{
			r = v;
			g = p;
			b = q;
		}
		else
		{
			r = v;
			g = t;
			b = p;
		}
	}
}

static void RGBtoHSV(float r, float g, float b, float & h, float & s, float & v)
{
	float max = (r > g  ?  r : g) > b  ?  (r > g  ?  r : g) : b;
	float min = (r < g  ?  r : g) < b  ?  (r < g  ?  r : g) : b;
	float delta = max - min;

	v = max;

	if (delta == 0)
	{
		h = 0;
		s = 0;
	}
	else
	{
		s = delta / max;

		if (r == max)
		{
			h = (g - b) / delta;
		}
		else if (g == max)
		{
			h = 2 + (b - r) / delta;
		}
		else
		{
			h = 4 + (r - g) / delta;
		}

		h = h / 6;

		if (h < 0)
		{
			h += 1;
		}
	}
}

WIDGET_COLORPICKER::WIDGET_COLORPICKER() :
	h_select(false),
	sv_select(false)
{
	// ctor
}

WIDGET * WIDGET_COLORPICKER::clone() const
{
	return new WIDGET_COLORPICKER(*this);
}

void WIDGET_COLORPICKER::SetAlpha(SCENENODE & scene, float newalpha)
{
	h_bar.SetAlpha(scene, newalpha);
	h_cursor.SetAlpha(scene, newalpha);
	sv_bg.SetAlpha(scene, newalpha);
	sv_plane.SetAlpha(scene, newalpha);
	sv_cursor.SetAlpha(scene, newalpha);
}

void WIDGET_COLORPICKER::SetVisible(SCENENODE & scene, bool newvis)
{
	h_bar.SetVisible(scene, newvis);
	h_cursor.SetVisible(scene, newvis);
	sv_bg.SetVisible(scene, newvis);
	sv_plane.SetVisible(scene, newvis);
	sv_cursor.SetVisible(scene, newvis);
}

void WIDGET_COLORPICKER::SetName(const std::string & newname)
{
	name = newname;
}

void WIDGET_COLORPICKER::SetDescription(const std::string & newdesc)
{
	description = newdesc;
}

std::string WIDGET_COLORPICKER::GetDescription() const
{
	return description;
}

void WIDGET_COLORPICKER::AddHook(WIDGET * other)
{
	hooks.push_back(other);
}

bool WIDGET_COLORPICKER::ProcessInput(SCENENODE & scene, float x, float y, bool down, bool justup)
{
	if (!down)
	{
		h_select = false;
		sv_select = false;
		return false;
	}

	if (!h_select &&
		x > sv_min[0] - size2 &&
		x < sv_max[0] + size2 &&
		y > sv_min[1] - size2 &&
		y < sv_max[1] + size2)
	{
		h_select = false;
		sv_select = true;

		if (x > sv_max[0]) x = sv_max[0];
		else if (x < sv_min[0]) x = sv_min[0];

		if (y > sv_max[1]) y = sv_max[1];
		else if (y < sv_min[1]) y = sv_min[1];

		float h = hsv[0];
		float s = (x - sv_min[0]) / (sv_max[0] - sv_min[0]);
		float v = (sv_max[1] - y) / (sv_max[1] - sv_min[1]);

		hsv.Set(h, s, v);
		HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
		UpdatePosition();

		std::stringstream str;
		str << rgb;
		SendMessage(scene, str.str());

		return true;
	}

	if (!sv_select &&
		x > h_min[0] - size2 &&
		x < h_max[0] + size2 &&
		y > h_min[1] - size2 &&
		y < h_max[1] + size2)
	{
		h_select = true;
		sv_select = false;

		if (y > h_max[1]) y = h_max[1];
		else if (y < h_min[1]) y = h_min[1];

		float h = (y - h_min[1]) / (h_max[1] - h_min[1]);
		float s = hsv[1];
		float v = hsv[2];

		float r, g, b;
		HSVtoRGB(hsv[0], 1, 1, r, g, b);
		sv_bg.SetColor(scene, r, g, b);

		hsv.Set(h, s, v);
		HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
		UpdatePosition();

		std::stringstream str;
		str << rgb;
		SendMessage(scene, str.str());

		return true;
	}

	return false;
}

void WIDGET_COLORPICKER::UpdateOptions(
	SCENENODE & scene,
	bool save_to_options,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	if (setting.empty()) return;

	if (save_to_options)
	{
		std::stringstream s;
		s << rgb;
		std::string value = s.str();
		optionmap[setting].SetCurrentValue(value);
	}
	else
	{
		std::string value = optionmap[setting].GetCurrentStorageValue();
		if (!value.empty())
		{
			std::stringstream s(value);
			s >> rgb;

			RGBtoHSV(rgb[0], rgb[1], rgb[2], hsv[0], hsv[1], hsv[2]);

			float r, g, b;
			HSVtoRGB(hsv[0], 1, 1, r, g, b);
			sv_bg.SetColor(scene, r, g, b);

			UpdatePosition();

			SendMessage(scene, value);
		}
	}
}

void WIDGET_COLORPICKER::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> cursortex,
	std::tr1::shared_ptr<TEXTURE> htex,
	std::tr1::shared_ptr<TEXTURE> svtex,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	float x, float y, float w, float h,
	const std::string & set,
	std::ostream & error_output,
	int draworder)
{
	assert(htex);
	assert(svtex);
	assert(cursortex);

	setting = set;

	sv_bg.Load(scene, bgtex, draworder-1);
	sv_plane.Load(scene, svtex, draworder);
	h_bar.Load(scene, htex, draworder);
	sv_cursor.Load(scene, cursortex, draworder+1);
	h_cursor.Load(scene, cursortex, draworder+1);

	size2 = h / 16;
	h_min[0] = x + w - 3 * size2;
	h_min[1] = y + size2;
	h_max[0] = x + w - size2;
	h_max[1] = y + h - size2;
	sv_min[0] = x + size2;
	sv_min[1] = y + size2;
	sv_max[0] = x + w - 4 * size2;
	sv_max[1] = y + h - size2;

	sv_bg.SetToBillboard(sv_min[0], sv_min[1], sv_max[0] - sv_min[0], sv_max[1] - sv_min[1]);
	sv_plane.SetToBillboard(sv_min[0], sv_min[1], sv_max[0] - sv_min[0], sv_max[1] - sv_min[1]);
	h_bar.SetToBillboard(h_min[0] + size2/2, h_min[1], h_max[0] - h_min[0] - size2, h_max[1] - h_min[1]);

	hsv.Set(1, 0, 1);
	HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);

	float r, g, b;
	HSVtoRGB(hsv[0], 1, 1, r, g, b);
	sv_bg.SetColor(scene, r, g, b);
	sv_plane.SetColor(scene, 1, 1, 1);

	UpdatePosition();
}

void WIDGET_COLORPICKER::UpdatePosition()
{
	h_pos[0] = h_max[0] - size2;
	h_pos[1] = h_min[1] + hsv[0] * (h_max[1] - h_min[1]);

	sv_pos[0] = sv_min[0] + hsv[1] * (sv_max[0] - sv_min[0]);
	sv_pos[1] = sv_max[1] - hsv[2] * (sv_max[1] - sv_min[1]);

	h_cursor.SetToBillboard(h_pos[0] - size2, h_pos[1] - size2, 2*size2, 2*size2);
	sv_cursor.SetToBillboard(sv_pos[0] - size2, sv_pos[1] - size2, 2*size2, 2*size2);
}

void WIDGET_COLORPICKER::SendMessage(SCENENODE & scene, const std::string & message) const
{
	for (std::list <WIDGET *>::const_iterator n = hooks.begin(); n != hooks.end(); n++)
	{
		(*n)->HookMessage(scene, message, name);
	}
}
