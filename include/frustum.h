#ifndef _FRUSTUM_H
#define _FRUSTUM_H

#include "mathvector.h"
#include "mathplane.h"

#include <iostream>

/*template <typename T>
class FRUSTUM
{
public:
	FRUSTUM() : planes(6) {}
	FRUSTUM(T cfrustum[][4]) : planes(6)
	{
		Set(cfrustum);
	}
	void Set(T cfrustum[][4])
	{
		for (int i = 0; i < 6; i++)
			planes[i].Set(cfrustum[i]);
	}
	
	void DebugPrint(std::ostream & output)
	{
		for (int i = 0; i < 6; i++)
			output << i << ". " << planes[i] << "\n";
	}
	
	inline const MATHPLANE <T> & operator[](const int n) const
	{
		assert(n >= 0);
		assert(n < 6);
		return planes[n];
	}

	inline MATHPLANE <T> & operator[](const int n)
	{
		assert(n >= 0);
		assert(n < 6);
		return planes[n];
	}
	
private:
	std::vector <MATHPLANE <T> > planes;
};*/

//typedef float FRUSTUM[6][4];

struct FRUSTUM
{
public:
	FRUSTUM() {}
	FRUSTUM(float cfrustum[][4])
	{
		Set(cfrustum);
	}
	void Set(float cfrustum[][4])
	{
		for (int i = 0; i < 6; i++)
			for (int n = 0; n < 4; n++)
				frustum[i][n] = cfrustum[i][n];
	}
	
	float frustum[6][4];
};

#endif
