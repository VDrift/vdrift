#include "gui/guicolorpicker.h"
#include "gui/guioption.h"
#include "mathvector.h"
#include "hsvtorgb.h"

GUICOLORPICKER::GUICOLORPICKER() :
	h_select(false),
	sv_select(false),
	update(false)
{
	set_value.call.bind<GUICOLORPICKER, &GUICOLORPICKER::SetValue>(this);
}

GUICOLORPICKER::~GUICOLORPICKER()
{
	// dtor
}

void GUICOLORPICKER::SetAlpha(SCENENODE & scene, float newalpha)
{
	h_bar.SetAlpha(scene, newalpha);
	h_cursor.SetAlpha(scene, newalpha);
	sv_bg.SetAlpha(scene, newalpha);
	sv_plane.SetAlpha(scene, newalpha);
	sv_cursor.SetAlpha(scene, newalpha);
}

void GUICOLORPICKER::SetVisible(SCENENODE & scene, bool newvis)
{
	h_bar.SetVisible(scene, newvis);
	h_cursor.SetVisible(scene, newvis);
	sv_bg.SetVisible(scene, newvis);
	sv_plane.SetVisible(scene, newvis);
	sv_cursor.SetVisible(scene, newvis);
}

void GUICOLORPICKER::Update(SCENENODE & scene, float dt)
{
	if (update)
	{
		float r, g, b;
		HSVtoRGB(hsv[0], 1, 1, r, g, b);
		sv_bg.SetColor(scene, r, g, b);
		UpdatePosition();
		update = false;
	}
}

bool GUICOLORPICKER::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	if (!cursordown)
	{
		h_select = false;
		sv_select = false;
		return false;
	}

	if (!h_select &&
		cursorx > sv_min[0] - size2 &&
		cursorx < sv_max[0] + size2 &&
		cursory > sv_min[1] - size2 &&
		cursory < sv_max[1] + size2)
	{
		h_select = false;
		sv_select = true;

		if (cursorx > sv_max[0]) cursorx = sv_max[0];
		else if (cursorx < sv_min[0]) cursorx = sv_min[0];

		if (cursory > sv_max[1]) cursory = sv_max[1];
		else if (cursory < sv_min[1]) cursory = sv_min[1];

		hsv[1] = (cursorx - sv_min[0]) / (sv_max[0] - sv_min[0]);
		hsv[2] = (sv_max[1] - cursory) / (sv_max[1] - sv_min[1]);
	}
	else if (!sv_select &&
		cursorx > h_min[0] - size2 &&
		cursorx < h_max[0] + size2 &&
		cursory > h_min[1] - size2 &&
		cursory < h_max[1] + size2)
	{
		h_select = true;
		sv_select = false;

		if (cursory > h_max[1]) cursory = h_max[1];
		else if (cursory < h_min[1]) cursory = h_min[1];

		hsv[0] = (cursory - h_min[1]) / (h_max[1] - h_min[1]);
	}

	float r, g, b;
	HSVtoRGB(hsv[0], hsv[1], hsv[2], r, g, b);
	unsigned rgbnew = packRGB(r, g, b);
	if (rgbnew != rgb)
	{
		std::stringstream s;
		s << rgbnew;
		signal_value(s.str());
		OnSelect();
		return true;
	}
	return false;
}

void GUICOLORPICKER::SetupDrawable(
	SCENENODE & scene,
	std::tr1::shared_ptr<TEXTURE> cursortex,
	std::tr1::shared_ptr<TEXTURE> htex,
	std::tr1::shared_ptr<TEXTURE> svtex,
	std::tr1::shared_ptr<TEXTURE> bgtex,
	float x, float y, float w, float h,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & nsetting,
	std::ostream & error_output,
	int draworder)
{
	assert(htex);
	assert(svtex);
	assert(cursortex);

	setting = nsetting;

	sv_bg.Load(scene, bgtex, draworder, error_output);
	sv_plane.Load(scene, svtex, draworder+1, error_output);
	h_bar.Load(scene, htex, draworder, error_output);
	sv_cursor.Load(scene, cursortex, draworder+2, error_output);
	h_cursor.Load(scene, cursortex, draworder+2, error_output);

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
	float r, g, b;
	HSVtoRGB(hsv[0], hsv[1], hsv[2], r, g, b);
	rgb = packRGB(r, g, b);

	HSVtoRGB(hsv[0], 1, 1, r, g, b);
	sv_bg.SetColor(scene, r, g, b);
	sv_plane.SetColor(scene, 1, 1, 1);
	UpdatePosition();

	// connect slot
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(setting);
	if (i != optionmap.end())
	{
		i->second.set_val.connect(signal_value);
		set_value.connect(i->second.signal_val);
	}
}

void GUICOLORPICKER::SetValue(const std::string & value)
{
	unsigned rgbnew;
	std::stringstream s(value);
	s >> rgbnew;
	if (rgb != rgbnew)
	{
		rgb = rgbnew;
		float r, g, b;
		unpackRGB(rgb, r, g, b);
		RGBtoHSV(r, g, b, hsv[0], hsv[1], hsv[2]);
		update = true;
	}
}

void GUICOLORPICKER::UpdatePosition()
{
	h_pos[0] = h_max[0] - size2;
	h_pos[1] = h_min[1] + hsv[0] * (h_max[1] - h_min[1]);

	sv_pos[0] = sv_min[0] + hsv[1] * (sv_max[0] - sv_min[0]);
	sv_pos[1] = sv_max[1] - hsv[2] * (sv_max[1] - sv_min[1]);

	h_cursor.SetToBillboard(h_pos[0] - size2, h_pos[1] - size2, 2*size2, 2*size2);
	sv_cursor.SetToBillboard(sv_pos[0] - size2, sv_pos[1] - size2, 2*size2, 2*size2);
}
