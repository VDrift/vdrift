#include "renderdimensions.h"

std::ostream & operator<<(std::ostream & out, const RenderDimensions & dim)
{
	out << "currently " << dim.width << "x" << dim.height;
	if (dim.origMultiples)
		out << ", automatically scales to " << dim.origw << "x" << dim.origh << " times the framebuffer size";
	return out;
}