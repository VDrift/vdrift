#ifndef _FRUSTUM_H
#define _FRUSTUM_H

#include "mathvector.h"
#include "mathplane.h"

#include <iostream>

template <typename T>
class AABB;

template <typename T>
class FRUSTUM
{
friend class AABB<T>;
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
};

#endif
