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

#include "aabb.h"
#include "mathvector.h"
#include "unittest.h"

template <typename T>
AABB<T>::AABB() : radius(0)
{
	// Constructor.
}

template <typename T>
AABB<T>::AABB(const AABB <T> & other) : pos(other.pos), center(other.center), size(other.size), radius(other.radius)
{
	// Constructor.
}

template <typename T>
const AABB <T> & AABB<T>::operator = (const AABB <T> & other)
{
	pos = other.pos;
	center = other.center;
	size = other.size;
	radius = other.radius;

	return *this;
}

template <typename T>
const MATHVECTOR <T, 3> & AABB<T>::GetPos() const
{
	return pos;
}

template <typename T>
const MATHVECTOR <T, 3> & AABB<T>::GetSize() const
{
	return size;
}

template <typename T>
const MATHVECTOR <T, 3> & AABB<T>::GetCenter() const
{
	return center;
}

template <typename T>
void AABB<T>::DebugPrint(std::ostream & o) const
{
	MATHVECTOR <T, 3> min(pos);
	MATHVECTOR <T, 3> max(pos+size);
	o << min[0] << "," << min[1] << "," << min[2] << " to " << max[0] << "," << max[1] << "," << max[2] << std::endl;
}

template <typename T>
void AABB<T>::DebugPrint2(std::ostream & o) const
{
	o << "center: " << center[0] << "," << center[1] << "," << center[2] << " size: " << size[0] << "," << size[1] << "," << size[2] << std::endl;
}

template <typename T>
void AABB<T>::SetFromSphere(const MATHVECTOR <T, 3> & newcenter, float newRadius)
{
	center = newcenter;
	size.Set(newRadius,newRadius,newRadius);
	size = size * 2.0;
	pos = center - size*0.5;
	radius = newRadius;
}

template <typename T>
void AABB<T>::SetFromCorners(const MATHVECTOR <T, 3> & c1, const MATHVECTOR <T, 3> & c2)
{
	MATHVECTOR <T, 3> c1mod(c1);
	MATHVECTOR <T, 3> c2mod(c2);

	// Ensure c1 is smaller than c2.
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
	recomputeRadius();
}

template <typename T>
void AABB<T>::CombineWith(const AABB <T> & other)
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

template <typename T>
AABB<T>::RAY::RAY(const MATHVECTOR <T, 3> & neworig, const MATHVECTOR <T, 3> & newdir, T newseglen) : orig(neworig), dir(newdir), seglen(newseglen)
{
	// Constructor.
}

template <typename T>
enum AABB<T>::INTERSECTION AABB<T>::Intersect(const RAY & ray) const
{
	MATHVECTOR <T, 3> segdir(ray.dir * (0.5f * ray.seglen));
	MATHVECTOR <T, 3> seg_center(ray.orig + segdir);
	MATHVECTOR <T, 3> diff(seg_center - center);

	MATHVECTOR <T, 3> abs_segdir(segdir);
	abs_segdir.absify();
	MATHVECTOR <T, 3> abs_diff(diff);
	abs_diff.absify();
	T f = size[0] + abs_segdir[0];
	if (abs_diff[0] > f)
		return OUT;
	f = size[1] + abs_segdir[1];
	if (abs_diff[1] > f)
		return OUT;
	f = size[2] + abs_segdir[2];
	if (abs_diff[2] > f)
		return OUT;

	MATHVECTOR <T, 3> cross(segdir.cross(diff));

	MATHVECTOR <T, 3> abs_cross(cross);
	abs_cross.absify();

	f = size[1]*abs_segdir[2] + size[2]*abs_segdir[1];
	if (abs_cross[0] > f)
		return OUT;

	f = size[2]*abs_segdir[0] + size[0]*abs_segdir[2];
	if (abs_cross[1] > f)
		return OUT;

	f = size[0]*abs_segdir[1] + size[1]*abs_segdir[0];
	if (abs_cross[2] > f)
		return OUT;

	return INTERSECT;
}

template <typename T>
enum AABB<T>::INTERSECTION AABB<T>::Intersect(const AABB <T> & other) const
{
	MATHVECTOR <T, 3> otherc1 = other.GetPos();
	MATHVECTOR <T, 3> otherc2 = otherc1 + other.GetSize();

	MATHVECTOR <T, 3> c1 = pos;
	MATHVECTOR <T, 3> c2 = pos + size;

	// Bias checks for non-collisions.
	if (c1[0] > otherc2[0] || c2[0] < otherc1[0])
		return OUT;

	if (c1[2] > otherc2[2] || c2[2] < otherc1[2])
		return OUT;

	if (c1[1] > otherc2[1] || c2[1] < otherc1[1])
		return OUT;

	return INTERSECT;
}

template <typename T>
enum AABB<T>::INTERSECTION AABB<T>::Intersect(const FRUSTUM & frustum) const
{
	float rd;
	const float bound = radius;
	for (int i=0; i<6; i++)
	{
		rd = frustum.frustum[i][0]*center[0] + frustum.frustum[i][1]*center[1] + frustum.frustum[i][2]*center[2] + frustum.frustum[i][3];
		if (rd < -bound)
			// Fully out.
			return OUT;
	}
	return INTERSECT;
}

template <typename T>
enum AABB<T>::INTERSECTION AABB<T>::Intersect(INTERSECT_ALWAYS always) const
{
	return IN;
}

template <typename T>
void AABB<T>::recomputeRadius()
{
	radius = size.Magnitude()*0.5;
}

// This is used for olbying the compiler to generate the corresponding float class for avoiding linker errors.
template class AABB<float>;

static void distribute(float frustum[][4])
{
	for (int i = 1; i < 6; i++)
		for (int n = 0; n < 4; n++)
			frustum[i][n] = frustum[0][n];
}

QT_TEST(aabb_test)
{
	AABB <float> box1;
	AABB <float> box2;

	MATHVECTOR <float, 3> c1;
	MATHVECTOR <float, 3> c2;

	c1.Set(-1,-1,-1);
	c2.Set(1,1,1);
	box1.SetFromCorners(c1, c2);

	c1.Set(-0.01, -0.01, 0);
	c2.Set(0.01,0.01, 2);
	box2.SetFromCorners(c1, c2);

	QT_CHECK(box1.Intersect(box2));

	AABB <float> box3;
	c1.Set(-0.01, -0.01, 2);
	c2.Set(0.01,0.01, 3);
	box3.SetFromCorners(c1, c2);

	QT_CHECK(!box1.Intersect(box3));

	MATHVECTOR <float, 3> orig;
	MATHVECTOR <float, 3> dir;
	orig.Set(0,0,4);
	dir.Set(0,0,-1);

	QT_CHECK(box1.Intersect(AABB<float>::RAY(orig,dir,4)));
	QT_CHECK(!box1.Intersect(AABB<float>::RAY(orig,dir*-1,4)));
	QT_CHECK(!box1.Intersect(AABB<float>::RAY(orig,dir,1)));

	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = 10;
		distribute(plane);
		QT_CHECK(box1.Intersect(FRUSTUM(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = 0;
		distribute(plane);
		QT_CHECK(box1.Intersect(FRUSTUM(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = -10;
		distribute(plane);
		QT_CHECK(!box1.Intersect(FRUSTUM(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = -1;
		plane[0][1] = 0;
		plane[0][2] = 0;
		plane[0][3] = 10000;
		distribute(plane);
		QT_CHECK(box1.Intersect(FRUSTUM(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 1;
		plane[0][1] = 0;
		plane[0][2] = 0;
		plane[0][3] = -119;
		distribute(plane);
		QT_CHECK(!box1.Intersect(FRUSTUM(plane)));
	}
}
