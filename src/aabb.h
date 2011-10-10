#ifndef _AABB_H
#define _AABB_H

#include "mathvector.h"
#include "mathplane.h"
#include "frustum.h"

#include <ostream>

template <typename T>
class AABB
{
private:
	MATHVECTOR <T, 3> pos; ///< minimum corner (center-size*0.5)
	MATHVECTOR <T, 3> center; ///< exact center of AABB
	MATHVECTOR <T, 3> size; ///< size of AABB
	float radius; ///< size.Magnitude()*0.5

	void recomputeRadius()
	{
		radius = size.Magnitude()*0.5;
	}

public:
	AABB(const AABB <T> & other) : pos(other.pos), center(other.center), size(other.size), radius(other.radius) {}
	AABB() : radius(0) {}

	const AABB <T> & operator = (const AABB <T> & other)
	{
		pos = other.pos;
		center = other.center;
		size = other.size;
		radius = other.radius;

		return *this;
	}

	const MATHVECTOR <T, 3> & GetPos() const {return pos;}
	const MATHVECTOR <T, 3> & GetSize() const {return size;}
	const MATHVECTOR <T, 3> & GetCenter() const {return center;}

	void DebugPrint(std::ostream & o) const
	{
		MATHVECTOR <T, 3> min(pos);
		MATHVECTOR <T, 3> max(pos+size);
		o << min[0] << "," << min[1] << "," << min[2] << " to " << max[0] << "," << max[1] << "," << max[2] << std::endl;
	}

	void DebugPrint2(std::ostream & o) const
	{
		o << "center: " << center[0] << "," << center[1] << "," << center[2] << " size: " << size[0] << "," << size[1] << "," << size[2] << std::endl;
	}

	void SetFromSphere(const MATHVECTOR <T, 3> & newcenter, float newRadius)
	{
		center = newcenter;
		size.Set(newRadius,newRadius,newRadius);
		size = size * 2.0;
		pos = center - size*0.5;
		radius = newRadius;
	}

	void SetFromCorners(const MATHVECTOR <T, 3> & c1, const MATHVECTOR <T, 3> & c2)
	{
		MATHVECTOR <T, 3> c1mod(c1);
		MATHVECTOR <T, 3> c2mod(c2);

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

		/*float fluff = 0.0001;
		VERTEX fluffer;
		fluffer.Set(fluff, fluff, fluff);

		//add in some fudge factor
		c1mod = c1mod + fluffer.ScaleR(-1.0);
		c2mod = c2mod + fluffer;*/

		pos = c1mod;
		size = c2mod - c1mod;
		center = pos + size * 0.5;
		recomputeRadius();
	}

	void CombineWith(const AABB <T> & other)
	{
		const MATHVECTOR <T, 3> & otherpos(other.GetPos());
		MATHVECTOR <T, 3> min(0), max(0);
		min = pos;
		max = pos + size;
		if (otherpos[0] < min[0])
			min[0] = otherpos[0];
		if (otherpos[1] < min[1])
			min[1] = otherpos[1];
		if (otherpos[2] < min[2])
			min[2] = otherpos[2];

		const MATHVECTOR <T, 3> & othermax(otherpos + other.GetSize());
		if (othermax[0] > max[0])
			max[0] = othermax[0];
		if (othermax[1] > max[1])
			max[1] = othermax[1];
		if (othermax[2] > max[2])
			max[2] = othermax[2];

		SetFromCorners(min, max);
	}

	// for intersection test returns
	enum INTERSECTION
	{
		OUT,
		INTERSECT,
		IN
	};

	class RAY
	{
		public:
			RAY(const MATHVECTOR <T, 3> & neworig, const MATHVECTOR <T, 3> & newdir, T newseglen) : orig(neworig), dir(newdir), seglen(newseglen) {}

			MATHVECTOR <T, 3> orig;
			MATHVECTOR <T, 3> dir;
			T seglen;
	};

	INTERSECTION Intersect(const RAY & ray) const
	{
		//if (seglen>3e30f) return IntersectRay(orig, dir); // infinite ray
		MATHVECTOR <T, 3> segdir(ray.dir * (0.5f * ray.seglen));
		MATHVECTOR <T, 3> seg_center(ray.orig + segdir);
		MATHVECTOR <T, 3> diff(seg_center - center);

		MATHVECTOR <T, 3> abs_segdir(segdir);
		abs_segdir.absify();
		MATHVECTOR <T, 3> abs_diff(diff);
		abs_diff.absify();
		T f = size[0] + abs_segdir[0];
		if (abs_diff[0] > f) return OUT;
		f = size[1] + abs_segdir[1];
		if (abs_diff[1] > f) return OUT;
		f = size[2] + abs_segdir[2];
		if (abs_diff[2] > f) return OUT;

		MATHVECTOR <T, 3> cross(segdir.cross(diff));

		MATHVECTOR <T, 3> abs_cross(cross);
		abs_cross.absify();

		f = size[1]*abs_segdir[2] + size[2]*abs_segdir[1];
		if ( abs_cross[0] > f ) return OUT;

		f = size[2]*abs_segdir[0] + size[0]*abs_segdir[2];
		if ( abs_cross[1] > f ) return OUT;

		f = size[0]*abs_segdir[1] + size[1]*abs_segdir[0];
		if ( abs_cross[2] > f ) return OUT;

		return INTERSECT;
	}

	INTERSECTION Intersect(const AABB <T> & other) const
	{
		MATHVECTOR <T, 3> otherc1 = other.GetPos();
		MATHVECTOR <T, 3> otherc2 = otherc1 + other.GetSize();

		MATHVECTOR <T, 3> c1 = pos;
		MATHVECTOR <T, 3> c2 = pos + size;

		//bias checks for non-collisions
		if (c1[0] > otherc2[0] || c2[0] < otherc1[0])
			return OUT;

		if (c1[2] > otherc2[2] || c2[2] < otherc1[2])
			return OUT;

		if (c1[1] > otherc2[1] || c2[1] < otherc1[1])
			return OUT;

		return INTERSECT;
	}

	INTERSECTION Intersect(const FRUSTUM & frustum) const
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

	struct INTERSECT_ALWAYS{};
	INTERSECTION Intersect(INTERSECT_ALWAYS always) const {return IN;}
};

#endif
