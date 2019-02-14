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

#ifndef _RENDERDIMENSIONS
#define _RENDERDIMENSIONS

#include <cmath>

class RenderDimensions
{
public:
	// Constructors.
	RenderDimensions();
	RenderDimensions(float w, float h);
	RenderDimensions(float w, float h, bool mult);

	/// Given the viewport size defined by w and h, put the calculated dimensions into outw and outh.
	/// Returns true if the calculated width and height have changed since the last call.
	bool get(unsigned int w, unsigned int h, unsigned int & outw, unsigned int & outh);

private:
	unsigned int width, height;
	float origw, origh;
	bool origMultiples;

	template <class Stream>
	friend Stream & operator<<(Stream & out, const RenderDimensions & dim);
};


inline RenderDimensions::RenderDimensions() :
	width(0), height(0), origw(0), origh(0), origMultiples(false)
{
	// ctor
}

inline RenderDimensions::RenderDimensions(float w, float h) :
	width(0), height(0), origw(w), origh(h), origMultiples(false)
{
	// ctor
}

inline RenderDimensions::RenderDimensions(float w, float h, bool mult) :
	width(0), height(0), origw(w), origh(h), origMultiples(mult)
{
	// ctor
}

inline bool RenderDimensions::get(unsigned int w, unsigned int h, unsigned int & outw, unsigned int & outh)
{
	outw = (unsigned int)std::floor(origw);
	outh = (unsigned int)std::floor(origh);
	if (origMultiples)
	{
		outw = (unsigned int)std::floor(origw*w);
		outh = (unsigned int)std::floor(origh*h);
	}

	if (outw != width || outh != height)
	{
		width = outw;
		height = outh;
		return true;
	}
	else
		return false;
}

template <class Stream>
inline Stream & operator<<(Stream & out, const RenderDimensions & dim)
{
	out << "currently " << dim.width << "x" << dim.height;
	if (dim.origMultiples)
		out << ", automatically scales to " << dim.origw << "x" << dim.origh << " times the framebuffer size";
	return out;
}

#endif
