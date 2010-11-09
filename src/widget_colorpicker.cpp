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
        float i = std::floor(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - (s * f));
        float t = v * (1 - (s * (1 - f)));

        if (i == 0)
        {
            r = v;
            g = t;
            b = p;
        }
        else if (i == 1)
        {
            r = q;
            g = v;
            b = p;
        }
        else if (i == 2)
        {
            r = p;
            g = v;
            b = t;
        }
        else if (i == 3)
        {
            r = p;
            g = q;
            b = v;
        }
        else if (i == 4)
        {
            r = t;
            g = p;
            b = v;
        }
        else
        {
            r = v;
            g = p;
            b = q;
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

bool WIDGET_COLORPICKER::ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup)
{
	if (cursordown &&
		cursorx < max[0] &&
		cursorx > min[0] &&
		cursory < max[1] &&
		cursory > min[1])
	{
		float h = max[1] - min[1] - 2 * margin;
		float w = max[0] - min[0] - csize - 2 * margin;
		
		if (cursory < min[1] + margin) cursory = min[1] + margin;
		else if (cursory > max[1] - margin) cursory = max[1] - margin;
		
		if (cursorx > max[0] - csize)
		{
			// vertical hue bar
			h_cursor.SetToBillboard(max[0] - csize, cursory - csize / 2, csize, csize);
			hsv[0] = (cursory - min[1] - margin) / h;
			
			float r, g, b;
			HSVtoRGB(hsv[0], 1, 1, r, g, b);
			sv_bg.SetColor(scene, r, g, b);
		}
		else
		{
			if (cursorx < min[0] + margin) cursorx = min[0] + margin;
			else if (cursorx > max[0] - csize - margin) cursorx = max[0] - csize - margin;
			
			// saturation value plane
			sv_cursor.SetToBillboard(cursorx - csize / 2, cursory - csize / 2, csize, csize);
			
			// lower left corner is 0, 0
			hsv[1] = (cursorx - min[0] - margin) / w;
			hsv[2] = (max[1] - margin - cursory) / h;
		}
		
		HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
		std::stringstream s;
		s << rgb;
		SendMessage(scene, s.str());
		
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
			
			float h = max[1] - min[1] - 2 * margin;
			float w = max[0] - min[0] - csize - 2 * margin;
			
			float x0 = min[0] + margin - csize / 2 + hsv[1] * w;
			float y0 = max[1] - margin - csize / 2 - hsv[2] * h;
			sv_cursor.SetToBillboard(x0, y0, csize, csize);
			
			float x1 = max[0] - csize;
			float y1 = min[1] + margin - csize / 2 + hsv[0] * h;
			h_cursor.SetToBillboard(x1, y1, csize, csize);
			
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
	
	sv_bg.Load(scene, bgtex, draworder-1, error_output);
	sv_plane.Load(scene, svtex, draworder, error_output);
	h_bar.Load(scene, htex, draworder, error_output);
	sv_cursor.Load(scene, cursortex, draworder+1, error_output);
	h_cursor.Load(scene, cursortex, draworder+1, error_output);
	
	min[0] = x;
	min[1] = y;
	max[0] = x + w;
	max[1] = y + h;
	csize = h / 8;
	margin = csize / 3;
	
	float x0 = x + margin;
	float y0 = y + margin;
	float w0 = w - csize - 2 * margin;
	float h0 = h - 2 * margin;
	float x1 = x + w - csize + margin / 2;
	float w1 = csize - margin;
	
	sv_bg.SetToBillboard(x0, y0, w0, h0);
	sv_plane.SetToBillboard(x0, y0, w0, h0);
	h_bar.SetToBillboard(x1, y0, w1, h0);
	
	sv_cursor.SetToBillboard(x, y, csize, csize);
	h_cursor.SetToBillboard(x + w - csize, y, csize, csize);
	
	hsv.Set(1, 0, 1);
	HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
	
	sv_bg.SetColor(scene, rgb[0], rgb[1], rgb[2]);
	sv_plane.SetColor(scene, 1, 1, 1);
}

void WIDGET_COLORPICKER::SendMessage(SCENENODE & scene, const std::string & message) const
{
	for (std::list <WIDGET *>::const_iterator n = hooks.begin(); n != hooks.end(); n++)
	{
		(*n)->HookMessage(scene, message, name);
	}
}
