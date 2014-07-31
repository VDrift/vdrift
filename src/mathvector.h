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

#ifndef _MATHVECTOR_H
#define _MATHVECTOR_H

#include "joeserialize.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <cstring> // memcpy
#include <cassert>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

template <typename T, unsigned int dimension>
class MathVector
{
friend class joeserialize::Serializer;
private:
	T v[dimension];

public:
	typedef size_t size_type;

	MathVector()
	{
		for (size_type i = 0; i < dimension; ++i)
			v[i] = 0;
	}

	MathVector(const T& t)
	{
		for (size_type i = 0; i < dimension; ++i)
			v[i] = t;
	}
	MathVector(const MathVector <T, dimension> & other)
	{
		*this = other;
	}
	MathVector(const T x, const T y)
	{
		assert(dimension==2);
		v[0] = x;
		v[1] = y;
	}

	const T Magnitude() const
	{
		return sqrt(MagnitudeSquared());
	}
	const T MagnitudeSquared() const
	{
		T running_total(0);
		for (size_type i = 0; i < dimension; i++)
			running_total += v[i] * v[i];
		return running_total;
	}

	///set all vector values to val1
	void Set(const T & val1)
	{
		//assert(dimension == 1);
		for (size_type i = 0; i < dimension; ++i)
			v[i] = val1;
	}
	void Set(const T & val1, const T & val2)
	{
		assert(dimension == 2);
		v[0] = val1;
		v[1] = val2;
	}
	void Set(const T & val1, const T & val2, const T & val3)
	{
		assert(dimension == 3);
		v[0] = val1;
		v[1] = val2;
		v[2] = val3;
	}

	///careful, there's no way to check the bounds of the array
	void Set(const T * array_pointer)
	{
		for (size_t i = 0; i < dimension; i++)
			v[i] = array_pointer[i];
	}

	///return a normalized vector
	MathVector <T, dimension> Normalize() const
	{
		MathVector <T, dimension> output;

		const T mag = (Magnitude());

		assert(mag != 0);

		for (size_type i = 0; i < dimension; i++)
		{
			output[i] = v[i]/mag;
		}

		return output;
	}

	///return the scalar dot product between this and other
	const T dot(const MathVector <T, dimension> & other) const
	{
		T output(0);
		for (size_type i = 0; i < dimension; i++)
		{
			output += v[i]*other.v[i];
		}
		return output;
	}

	///return the cross product between this vector and the given vector
	const MathVector <T, 3> cross(const MathVector <T, 3> & other) const
	{
		assert(dimension==3);

		MathVector <T, 3> output;
		output[0] = v[1]*other.v[2] - v[2]*other.v[1];
		output[1] = v[2]*other.v[0] - v[0]*other.v[2];
		output[2] = v[0]*other.v[1] - v[1]*other.v[0];
		return output;
	}

	///return the reflection of this vector around the given normal (must be unit length)
	const MathVector <T, dimension> reflect(const MathVector <T, dimension> & other) const
	{
		MathVector <T, dimension> output;

		output = (*this)-other*T(2.0)*other.dot(*this);

		return output;
	}

	const T & operator[](size_type n) const
	{
		assert(n < dimension);
		return v[n];
	}

	T & operator[](size_type n)
	{
		assert(n < dimension);
		return v[n];
	}

	///scalar multiplication
	MathVector <T, dimension> operator * (const T & scalar) const
	{
		MathVector <T, dimension> output;

		for (size_type i = 0; i < dimension; i++)
		{
			output[i] = v[i]*scalar;
		}

		return output;
	}

	///scalar division
	MathVector <T, dimension> operator / (const T & scalar) const
	{
		assert(scalar != 0);

		MathVector <T, dimension> output;

		for (size_type i = 0; i < dimension; i++)
		{
			output[i] = v[i]/scalar;
		}

		return output;
	}

	MathVector <T, dimension> operator + (const MathVector <T, dimension> & other) const
	{
		MathVector <T, dimension> output;

		for (size_type i = 0; i < dimension; i++)
		{
			output[i] = v[i] + other.v[i];
		}

		return output;
	}

	MathVector <T, dimension> operator - (const MathVector <T, dimension> & other) const
	{
		MathVector <T, dimension> output;

		for (size_type i = 0; i < dimension; i++)
		{
			output[i] = v[i] - other.v[i];
		}

		return output;
	}

	template <typename T2>
	const MathVector <T, dimension> & operator = (const MathVector <T2, dimension> & other)
	{
		for (size_type i = 0; i < dimension; ++i)
			v[i] = other[i];

		return *this;
	}

	template <typename T2>
	bool operator== (const MathVector <T2, dimension> & other) const
	{
		bool same(true);

		for (size_type i = 0; i < dimension; i++)
		{
			same = same && (v[i] == other.v[i]);
		}

		return same;
	}

	template <typename T2>
	bool operator!= (const MathVector <T2, dimension> & other) const
	{
		return !(*this == other);
	}

	///inversion
	MathVector <T, dimension> operator-() const
	{
		MathVector <T, dimension> output;
		for (size_type i = 0; i < dimension; i++)
		{
			output.v[i] = -v[i];
		}
		return output;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		for (unsigned int i = 0; i < dimension; i++)
		{
			std::ostringstream namestr;
			namestr << "v" << i;
			if (!s.Serialize(namestr.str(),v[i])) return false;
		}
		return true;
	}
};

///we need a faster mathvector for 3-space, so specialize
template <class T>
class MathVector <T, 3>
{
friend class joeserialize::Serializer;
private:
	struct Vector3
	{
		T x,y,z;
		inline T & operator[](const int i) { return ((T*)this)[i]; }
		inline const T & operator[](const int i) const { return ((T*)this)[i]; }

		Vector3() : x(0), y(0), z(0) {}
		Vector3(const T& t) : x(t), y(t), z(t) {}
		Vector3(const T nx, const T ny, const T nz) : x(nx),y(ny),z(nz) {}
	} v;

public:
	MathVector()
	{
	}

	MathVector(const T& t) : v(t)
	{
	}

	MathVector(const T x, const T y, const T z) : v(x,y,z)
	{
	}

	template <typename T2>
	MathVector (const MathVector <T2, 3> & other)
	{
		*this = other;
	}

	inline const T Magnitude() const
	{
		return sqrt(MagnitudeSquared());
	}
	inline const T MagnitudeSquared() const
	{
		return v.x*v.x+v.y*v.y+v.z*v.z;
	}

	///set all vector values to val1
	inline void Set(const T val1)
	{
		v.x = v.y = v.z = val1;
	}

	inline void Set(const T val1, const T val2, const T val3)
	{
		v.x = val1;
		v.y = val2;
		v.z = val3;
	}

	///careful, there's no way to check the bounds of the array
	inline void Set(const T * array_pointer)
	{
		std::memcpy(&v,array_pointer,sizeof(Vector3)); //high performance, but portability issues?
		/*v.x = array_pointer[0];
		v.y = array_pointer[1];
		v.z = array_pointer[2];*/
	}

	///return a normalized vector
	MathVector <T, 3> Normalize() const
	{
		const T mag = Magnitude();
		assert(mag != 0);
		const T maginv = (1.0/mag);

		return MathVector <T, 3> (v.x*maginv, v.y*maginv, v.z*maginv);
	}

	///return the scalar dot product between this and other
	inline const T dot(const MathVector <T, 3> & other) const
	{
		return v.x*other.v.x+v.y*other.v.y+v.z*other.v.z;
	}

	///return the cross product between this vector and the given vector
	const MathVector <T, 3> cross(const MathVector <T, 3> & other) const
	{
		return MathVector <T, 3> (v[1]*other.v[2] - v[2]*other.v[1],
				v[2]*other.v[0] - v[0]*other.v[2],
				v[0]*other.v[1] - v[1]*other.v[0]);
	}

	///return the reflection of this vector around the given normal (must be unit length)
	const MathVector <T, 3> reflect(const MathVector <T, 3> & other) const
	{
		return (*this)-other*T(2.0)*other.dot(*this);
	}

	inline const T & operator[](const int n) const
	{
		assert(n < 3);
		return v[n];
	}

	inline T & operator[](const int n)
	{
		assert(n < 3);
		return v[n];
	}

	///scalar multiplication
	MathVector <T, 3> operator * (const T & scalar) const
	{
		return MathVector <T, 3> (v.x*scalar,v.y*scalar,v.z*scalar);
	}

	///scalar division
	MathVector <T, 3> operator / (const T & scalar) const
	{
		assert(scalar != 0);
		T invscalar = 1.0/scalar;
		return (*this)*invscalar;
	}

	MathVector <T, 3> operator + (const MathVector <T, 3> & other) const
	{
		return MathVector <T, 3> (v.x+other.v.x,v.y+other.v.y,v.z+other.v.z);
	}

	MathVector <T, 3> operator - (const MathVector <T, 3> & other) const
	{
		return MathVector <T, 3> (v.x-other.v.x,v.y-other.v.y,v.z-other.v.z);;
	}

	template <typename T2>
	const MathVector <T, 3> & operator = (const MathVector <T2, 3> & other)
	{
		v.x = other[0];
		v.y = other[1];
		v.z = other[2];

		return *this;
	}

	template <typename T2>
	inline bool operator== (const MathVector <T2, 3> & other) const
	{
		//return (std::memcmp(&v,&other.v,sizeof(MATHVECTOR_XYZ)) == 0);
		return (v.x == other[0] && v.y == other[1] && v.z == other[2]);
	}

	template <typename T2>
	inline bool operator!= (const MathVector <T2, 3> & other) const
	{
		return !(*this == other);
	}

	///inversion
	MathVector <T, 3> operator-() const
	{
		return MathVector <T, 3> (-v.x, -v.y, -v.z);
	}

	///set all vector components to be positive
	inline void absify()
	{
		v.x = fabs(v.x);
		v.y = fabs(v.y);
		v.z = fabs(v.z);
	}

	///project this vector onto the vector 'vec'.  neither needs to be a unit vector
	MathVector <T, 3> project(const MathVector <T, 3> & vec) const
	{
		T scalar_projection = dot(vec.Normalize());
		return vec.Normalize() * scalar_projection;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		if (!s.Serialize("x",v.x)) return false;
		if (!s.Serialize("y",v.y)) return false;
		if (!s.Serialize("z",v.z)) return false;
		return true;
	}
};

template <typename T, unsigned int dimension>
std::ostream & operator << (std::ostream &os, const MathVector <T, dimension> & v)
{
	for (size_t i = 0; i < dimension-1; i++)
	{
		os << v[i] << ", ";
	}
	os << v[dimension-1];
	return os;
}

template <typename T, unsigned int dimension>
std::istream & operator >> (std::istream &is, MathVector <T, dimension> & v)
{
	std::string value;
	for (size_t i = 0; i < dimension && std::getline(is, value, ','); ++i)
	{
		std::istringstream s(value);
		s >> v[i];
	}
	return is;
}

typedef MathVector <float, 2> Vec2;
typedef MathVector <float, 3> Vec3;
typedef MathVector <float, 4> Vec4;

#endif
