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

#ifndef _AABB_H
#define _AABB_H

#include "mathvector.h"
#include "minmax.h"

template <typename T>
class Aabb
{
public:
	Aabb() : radius(0) {}

	Aabb(const Aabb<T> & other) :
		center(other.center), extent(other.extent), radius(other.radius) {}

	Aabb(const MathVector<T, 3> & min, const MathVector<T, 3> & max)
	{
		center = (max + min) * T(0.5);
		extent = (max - min) * T(0.5);
		radius = extent.Magnitude();
	}

	Aabb<T> & operator=(const Aabb<T> & other)
	{
		center = other.center;
		extent = other.extent;
		radius = other.radius;
		return *this;
	}

	const MathVector<T, 3> & GetCenter() const
	{
		return center;
	}

	const MathVector<T, 3> & GetExtent() const
	{
		return extent;
	}

	T GetRadius() const
	{
		return radius;
	}

	void SetFromSphere(const MathVector<T, 3> & ncenter, float nradius)
	{
		center = ncenter;
		extent.Set(nradius, nradius, nradius);
		radius = nradius;
	}

	void SetFromCorners(const MathVector<T, 3> & c1, const MathVector<T, 3> & c2)
	{
		center = (c1 + c2) * T(0.5);
		extent = (c1 - c2) * T(0.5);
		extent.absify();
		radius = extent.Magnitude();
	}

	void CombineWith(const Aabb<T> & other)
	{
		auto min = center - extent;
		auto max = center + extent;
		auto omin = other.center - other.extent;
		auto omax = other.center + other.extent;
		for (int i = 0; i < 3; i++)
		{
			min[i] = Min(min[i], omin[i]);
			max[i] = Max(max[i], omax[i]);
		}
		*this = Aabb(min, max);
	}

	// for intersection test returns
	enum IntersectionEnum
	{
		OUT,
		INTERSECT,
		IN
	};

	struct IntersectAlways
	{
		// placebo
	};

	struct Ray
	{
		Ray(const MathVector<T, 3> & norig, const MathVector<T, 3> & ndir, T nseglen) :
			orig(norig), dir(ndir), seglen(nseglen) {}

		MathVector<T, 3> orig;
		MathVector<T, 3> dir;
		T seglen;
	};

	IntersectionEnum Intersect(const Ray & ray) const
	{
		auto d = ray.orig - center;

		// bounding box
		auto s = ray.seglen * T(0.5);
		for (int i = 0; i < 3; i++)
		{
			auto e = ray.dir[i] * s;
			if (std::abs(d[i] + e) > extent[i] + std::abs(e))
				return OUT;
		}

		// separating axis
		auto c = ray.dir.cross(d);
		auto a = ray.dir;
		a.absify();

		if (std::abs(c[0]) > extent[1] * a[2] + extent[2] * a[1])
			return OUT;

		if (std::abs(c[1]) > extent[0] * a[2] + extent[2] * a[0])
			return OUT;

		if (std::abs(c[2]) > extent[0] * a[1] + extent[1] * a[0])
			return OUT;

		return INTERSECT;
	}

	IntersectionEnum Intersect(const Aabb<T> & other) const
	{
		for (int i = 0; i < 3; i++)
		{
			auto d = center[i] - other.center[i];
			auto e = extent[i] + other.extent[i];
			if (std::abs(d) > e)
				return OUT;
		}
		return INTERSECT;
	}

	template <typename Culler>
	IntersectionEnum Intersect(const Culler & cull) const
	{
		return cull(center, extent, radius) ? OUT : INTERSECT;
	}

	IntersectionEnum Intersect(IntersectAlways /*always*/) const
	{
		return IN;
	}

	template <typename Stream>
	void DebugPrint(Stream & o) const
	{
		auto min = center - extent;
		auto max = center + extent;
		o << min[0] << "," << min[1] << "," << min[2] << " to "
			<< max[0] << "," << max[1] << "," << max[2] << "\n";
	}

	template <typename Stream>
	void DebugPrint2(Stream & o) const
	{
		o << "center: " << center[0] << "," << center[1] << "," << center[2]
			<< " extent: " << extent[0] << "," << extent[1] << "," << extent[2] << "\n";
	}

private:
	MathVector<T, 3> center; ///< (min + max) / 2
	MathVector<T, 3> extent; ///< (max - min) / 2
	T radius; ///< extent.Magnitude()
};

#endif
