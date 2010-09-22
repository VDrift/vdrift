#include "cartireinfo.h"

#include <sstream>

template <typename T>
bool CARTIRESIZE<T>::Parse(std::string size, std::ostream & error)
{
	T section_width(0), rim_diameter(0);
	std::string modsize = size;
	for (unsigned int i = 0; i < modsize.length(); i++)
	{
		if (modsize[i] < '0' || modsize[i] > '9')
			modsize[i] = ' ';
	}
	
	std::stringstream parser(modsize);
	parser >> section_width >> aspect_ratio >> rim_diameter;
	if (section_width <= 0 || aspect_ratio <= 0 || rim_diameter <= 0)
	{
		error << "Expected something like 225/50r16 but got: " << size << std::endl;
		return false;
	}
	
	radius = section_width * 0.001 * aspect_ratio * 0.01 + rim_diameter * 0.0254 * 0.5;
	aspect_ratio = aspect_ratio * 0.01;
	sidewall_width = section_width * 0.001;
	
	return true;
}

// explizit instantiation
template struct CARTIRESIZE <float>;
template struct CARTIRESIZE <double>;
template struct CARTIREINFO <float>;
template struct CARTIREINFO <double>;

