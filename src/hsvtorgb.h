/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

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

inline void HSVtoRGB(float hsv[3], float rgb[3])
{
	HSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
}

inline void RGBtoHSV(float rgb[3], float hsv[3])
{
	RGBtoHSV(rgb[0], rgb[1], rgb[2], hsv[0], hsv[1], hsv[2]);
}

inline unsigned PackRGB(float r, float g, float b)
{
	unsigned rgb = 0;
	rgb |= (unsigned(r * 255) & 255) << 16;
	rgb |= (unsigned(g * 255) & 255) << 8;
	rgb |= (unsigned(b * 255) & 255) << 0;
	return rgb;
}

inline void UnpackRGB(unsigned rgb, float & r, float & g, float & b)
{
	r = float((rgb >> 16) & 255) / 255;
	g = float((rgb >> 8) & 255) / 255;
	b = float((rgb >> 0) & 255) / 255;
}

#endif //_HSVTORGB_H
