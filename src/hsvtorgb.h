#ifndef _HSVTORGB_H
#define _HSVTORGB_H

#include <cmath>

inline void HSVtoRGB(float h, float s, float v, float & r, float & g, float & b)
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

inline void RGBtoHSV(float r, float g, float b, float & h, float & s, float & v)
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

inline unsigned packRGB(float r, float g, float b)
{
	unsigned rgb = 0;
	rgb |= (unsigned(r * 255) & 255) << 16;
	rgb |= (unsigned(g * 255) & 255) << 8;
	rgb |= (unsigned(b * 255) & 255) << 0;
	return rgb;
}

inline void unpackRGB(unsigned rgb, float & r, float & g, float & b)
{
	r = float((rgb >> 16) & 255) / 255;
	g = float((rgb >> 8) & 255) / 255;
	b = float((rgb >> 0) & 255) / 255;
}

#endif //_HSVTORGB_H
