#ifndef _RENDERDIMENSIONS
#define _RENDERDIMENSIONS

#include <cmath>
#include <iostream>

class RenderDimensions
{
	public:
		/// given the viewport size defined by w and h, put the calculated dimensions into outw and outh
		/// returns true if the calculated width and height have changed since the last call
		bool get(unsigned int w, unsigned int h, unsigned int & outw, unsigned int & outh)
		{
			outw = (unsigned int)floor(origw);
			outh = (unsigned int)floor(origh);
			if (origMultiples)
			{
				outw = (unsigned int)floor(origw*w);
				outh = (unsigned int)floor(origh*h);
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
		
		RenderDimensions() : width(0), height(0), origw(0), origh(0), origMultiples(false) {}
		RenderDimensions(float w, float h) : width(0), height(0), origw(w), origh(h), origMultiples(false) {}
		RenderDimensions(float w, float h, bool mult) : width(0), height(0), origw(w), origh(h), origMultiples(mult) {}
		
	private:
		unsigned int width, height;
		float origw, origh;
		bool origMultiples;
		
		friend std::ostream & operator<<(std::ostream & out, const RenderDimensions & dim);
};

#endif