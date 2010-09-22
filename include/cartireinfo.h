#ifndef _CARTIREINFO_H
#define _CARTIREINFO_H

#include <vector>
#include <string>
#include <ostream>

template <typename T>
struct CARTIRESIZE
{
	T radius; ///< the total tread radius of the tire in meters
	T aspect_ratio; ///< 0..1 percentage value
	T sidewall_width; /// in meters
	
	CARTIRESIZE() : radius(0.3), aspect_ratio(0.5), sidewall_width(0.185) {}
	
	bool Parse(std::string size, std::ostream & error);
};

template <typename T>
struct CARTIREINFO : CARTIRESIZE<T>
{
	T tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	T rolling_resistance_linear; ///< linear rolling resistance on a hard surface
	T rolling_resistance_quadratic; ///< quadratic rolling resistance on a hard surface
	std::vector <T> longitudinal; ///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector <T> lateral; ///< the parameters of the lateral pacejka equation.  this is series a
	std::vector <T> aligning; ///< the parameters of the aligning moment pacejka equation.  this is series c
	
	CARTIREINFO() : tread(0), longitudinal(11), lateral(15), aligning(18) {}
};

#endif // _CARTIREINFO_H
