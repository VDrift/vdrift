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

#ifndef _MATHPLANE_H
#define _MATHPLANE_H

#include "mathvector.h"

#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include <cstring>
#include <sstream>

template <class T>
class MathPlane
{
	private:
		struct Plane
		{
			T a,b,c,d;
			inline T & operator[](const int i) { return ((T*)this)[i]; }
			inline const T & operator[](const int i) const { return ((T*)this)[i]; }

			Plane() : a(0),b(1),c(0),d(0) {}
			Plane(const T na, const T nb, const T nc, const T nd) : a(na),b(nb),c(nc),d(nd) {}
		} v;

	public:
		MathPlane()
		{
		}

		MathPlane(const T a, const T b, const T c, const T d) : v(a,b,c,d)
		{
		}

		MathPlane(const MathPlane <T> & other)
		{
			std::memcpy(&v,&other.v,sizeof(Plane)); //high performance, but portability issues?
		}

		template <typename T2>
		MathPlane(const MathPlane <T2> & other)
		{
			*this = other;
		}

		inline void Set(const T a, const T b, const T c, const T d)
		{
			v = Plane(a,b,c,d);
		}

		/// set from a normal and point
		void Set(const MathVector <T, 3> & normal, const MathVector <T, 3> & point)
		{
			v.a = normal[0];
			v.b = normal[1];
			v.c = normal[2];
			v.d = -v.a*point[0]-v.b*point[1]-v.c*point[2];
		}

		/// set from the three corners of a triangle
		void Set(const MathVector <T, 3> & v0, const MathVector <T, 3> & v1, const MathVector <T, 3> & v2)
		{
			MathVector <T, 3> normal = (v1-v0).cross(v2-v0);
			Set(normal, v0);
		}

		///careful, there's no way to check the bounds of the array
		inline void Set(const T * array_pointer)
		{
			std::memcpy(&v,array_pointer,sizeof(Plane)); //high performance, but portability issues?
		}

		inline const T & operator[](const int n) const
		{
			assert(n >= 0);
			assert(n < 4);
			return v[n];
		}

		inline T & operator[](const int n)
		{
			assert(n >= 0);
			assert(n < 4);
			return v[n];
		}

		template <typename T2>
		MathPlane <T> & operator = (const MathPlane <T2> & other)
		{
			v.a = other[0];
			v.b = other[1];
			v.c = other[2];
			v.d = other[3];

			return *this;
		}

		template <typename T2>
		inline bool operator== (const MathPlane <T2> & other) const
		{
			return (v.a == other[0] && v.b == other[1] && v.c == other[2] && v.d == other[3]);
		}

		template <typename T2>
		inline bool operator!= (const MathPlane <T2> & other) const
		{
			return !(*this == other);
		}

		T DistanceToPoint(const MathVector <T, 3> & point) const
		{
			T abcsq = v.a*v.a+v.b*v.b+v.c*v.c;
			assert(abcsq != 0);
			return (v.a*point[0]+v.b*point[1]+v.c*point[2]+v.d)/sqrt(abcsq);
		}
};

template <typename T>
std::ostream & operator << (std::ostream &os, const MathPlane <T> & v)
{
	for (size_t i = 0; i < 3; i++)
	{
		os << v[i] << ", ";
	}
	os << v[3];// << std::endl;
	return os;
}

#endif
