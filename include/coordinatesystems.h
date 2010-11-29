#ifndef _COORDINATESYSTEMS_H
#define _COORDINATESYSTEMS_H

namespace COORDINATESYSTEMS
{
	template <typename T>
	void ConvertV2toV1(T & x, T & y, T & z)
	{
		T tempx = x;
		T tempy = y;
		T tempz = z;
		
		x = tempy;
		y = -tempx;
		z = tempz;
	}
}

#endif
