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
#include "frustum.h"
#include <ostream>

template <typename T>
class Aabb
{
public:
	Aabb(const Aabb <T> & other) :
		pos(other.pos), center(other.center), size(other.size), radius(other.radius)
	{
		// ctor
	}

	Aabb(const MathVector <T, 3> & min, const MathVector <T, 3> & max)
	{
		pos = min;
		center = (min + max) * 0.5;
		size = max - min;
		radius = size.Magnitude() * 0.5;
	}

	Aabb() : radius(0) {}

	Aabb <T> & operator = (const Aabb <T> & other)
	{
		pos = other.pos;
		center = other.center;
		size = other.size;
		radius = other.radius;
		return *this;
	}

	const MathVector <T, 3> & GetPos() const {return pos;}

	const MathVector <T, 3> & GetSize() const {return size;}

	const MathVector <T, 3> & GetCenter() const {return center;}

	void DebugPrint(std::ostream & o) const
	{
		MathVector <T, 3> min(pos);
		MathVector <T, 3> max(pos+size);
		o << min[0] << "," << min[1] << "," << min[2] << " to " << max[0] << "," << max[1] << "," << max[2] << std::endl;
	}

	void DebugPrint2(std::ostream & o) const
	{
		o << "center: " << center[0] << "," << center[1] << "," << center[2] << " size: " << size[0] << "," << size[1] << "," << size[2] << std::endl;
	}

	void SetFromSphere(const MathVector <T, 3> & newcenter, float newRadius)
	{
		center = newcenter;
		size.Set(newRadius,newRadius,newRadius);
		size = size * 2.0;
		pos = center - size*0.5;
		radius = newRadius;
	}

	void SetFromCorners(const MathVector <T, 3> & c1, const MathVector <T, 3> & c2)
	{
		MathVector <T, 3> c1mod(c1);
		MathVector <T, 3> c2mod(c2);

		//ensure c1 is smaller than c2
		if (c1[0] > c2[0])
		{
			c1mod[0] = c2[0];
			c2mod[0] = c1[0];
		}
		if (c1[1] > c2[1])
		{
			c1mod[1] = c2[1];
			c2mod[1] = c1[1];
		}
		if (c1[2] > c2[2])
		{
			c1mod[2] = c2[2];
			c2mod[2] = c1[2];
		}

		pos = c1mod;
		size = c2mod - c1mod;
		center = pos + size * 0.5;
		radius = size.Magnitude() * 0.5;
	}

	void CombineWith(const Aabb <T> & other)
	{
		const MathVector <T, 3> othermin = other.GetPos();
		const MathVector <T, 3> othermax = othermin + other.GetSize();

		MathVector <T, 3> min = pos;
		MathVector <T, 3> max = pos + size;

		if (othermin[0] < min[0])
			min[0] = othermin[0];
		if (othermin[1] < min[1])
			min[1] = othermin[1];
		if (othermin[2] < min[2])
			min[2] = othermin[2];

		if (othermax[0] > max[0])
			max[0] = othermax[0];
		if (othermax[1] > max[1])
			max[1] = othermax[1];
		if (othermax[2] > max[2])
			max[2] = othermax[2];

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

	class Ray
	{
		public:
			Ray(const MathVector <T, 3> & neworig, const MathVector <T, 3> & newdir, T newseglen) : orig(neworig), dir(newdir), seglen(newseglen) {}

			MathVector <T, 3> orig;
			MathVector <T, 3> dir;
			T seglen;
	};

	IntersectionEnum Intersect(const Ray & ray) const
	{
		//if (seglen>3e30f) return IntersectRay(orig, dir); // infinite ray
		MathVector <T, 3> segdir(ray.dir * (0.5f * ray.seglen));
		MathVector <T, 3> seg_center(ray.orig + segdir);
		MathVector <T, 3> diff(seg_center - center);

		MathVector <T, 3> abs_segdir(segdir);
		abs_segdir.absify();
		MathVector <T, 3> abs_diff(diff);
		abs_diff.absify();
		T f = size[0] + abs_segdir[0];
		if (abs_diff[0] > f) return OUT;
		f = size[1] + abs_segdir[1];
		if (abs_diff[1] > f) return OUT;
		f = size[2] + abs_segdir[2];
		if (abs_diff[2] > f) return OUT;

		MathVector <T, 3> cross(segdir.cross(diff));

		MathVector <T, 3> abs_cross(cross);
		abs_cross.absify();

		f = size[1]*abs_segdir[2] + size[2]*abs_segdir[1];
		if ( abs_cross[0] > f ) return OUT;

		f = size[2]*abs_segdir[0] + size[0]*abs_segdir[2];
		if ( abs_cross[1] > f ) return OUT;

		f = size[0]*abs_segdir[1] + size[1]*abs_segdir[0];
		if ( abs_cross[2] > f ) return OUT;

		return INTERSECT;
	}

	IntersectionEnum Intersect(const Aabb <T> & other) const
	{
		MathVector <T, 3> otherc1 = other.GetPos();
		MathVector <T, 3> otherc2 = otherc1 + other.GetSize();

		MathVector <T, 3> c1 = pos;
		MathVector <T, 3> c2 = pos + size;

		//bias checks for non-collisions
		if (c1[0] > otherc2[0] || c2[0] < otherc1[0])
			return OUT;

		if (c1[2] > otherc2[2] || c2[2] < otherc1[2])
			return OUT;

		if (c1[1] > otherc2[1] || c2[1] < otherc1[1])
			return OUT;

		return INTERSECT;
	}

	IntersectionEnum Intersect(const Frustum & frustum) const
	{
		float rd;
		const float bound = radius;
		//INTERSECTION intersection = IN; // assume we are fully in until we find an intersection
		for (int i=0; i<6; i++)
		{
			rd=frustum.frustum[i][0]*center[0]+
					frustum.frustum[i][1]*center[1]+
					frustum.frustum[i][2]*center[2]+
					frustum.frustum[i][3];
			if (rd < -bound)
			{
				// fully out
				return OUT;
			}

			/*if (fabs(rd) < bound)
			{
				// partially in
				// we don't return here because we could still be fully out of another frustum plane
				intersection = INTERSECT;
			}*/
		}

		//return intersection;
		return INTERSECT;
	}

	IntersectionEnum Intersect(IntersectAlways /*always*/) const
	{
		return IN;
	}

private:
	MathVector <T, 3> pos; ///< minimum corner (center-size*0.5)
	MathVector <T, 3> center; ///< exact center of AABB
	MathVector <T, 3> size; ///< size of AABB
	float radius; ///< size.Magnitude()*0.5
};

#endif
