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

static void distribute(float frustum[][4])
{
	for (int i = 1; i < 6; i++)
		for (int n = 0; n < 4; n++)
			frustum[i][n] = frustum[0][n];
}

QT_TEST(aabb_test)
{
	Aabb <float> box1;
	Aabb <float> box2;

	Vec3 c1;
	Vec3 c2;

	c1.Set(-1,-1,-1);
	c2.Set(1,1,1);
	box1.SetFromCorners(c1, c2);

	c1.Set(-0.01, -0.01, 0);
	c2.Set(0.01,0.01, 2);
	box2.SetFromCorners(c1, c2);

	QT_CHECK(box1.Intersect(box2));

	Aabb <float> box3;
	c1.Set(-0.01, -0.01, 2);
	c2.Set(0.01,0.01, 3);
	box3.SetFromCorners(c1, c2);

	QT_CHECK(!box1.Intersect(box3));

	Vec3 orig;
	Vec3 dir;
	orig.Set(0,0,4);
	dir.Set(0,0,-1);

	//QT_CHECK(box1.IntersectRay(orig,dir));
	//QT_CHECK(!box1.IntersectRay(orig,dir*-1));

	QT_CHECK(box1.Intersect(Aabb<float>::Ray(orig,dir,4)));
	QT_CHECK(!box1.Intersect(Aabb<float>::Ray(orig,dir*-1,4)));
	QT_CHECK(!box1.Intersect(Aabb<float>::Ray(orig,dir,1)));

	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = 10;
		distribute(plane);
		QT_CHECK(box1.Intersect(Frustum(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = 0;
		distribute(plane);
		QT_CHECK(box1.Intersect(Frustum(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 0;
		plane[0][1] = 0;
		plane[0][2] = 1;
		plane[0][3] = -10;
		distribute(plane);
		QT_CHECK(!box1.Intersect(Frustum(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = -1;
		plane[0][1] = 0;
		plane[0][2] = 0;
		plane[0][3] = 10000;
		distribute(plane);
		QT_CHECK(box1.Intersect(Frustum(plane)));
	}
	{
		float plane[6][4];
		plane[0][0] = 1;
		plane[0][1] = 0;
		plane[0][2] = 0;
		plane[0][3] = -119;
		distribute(plane);
		QT_CHECK(!box1.Intersect(Frustum(plane)));
	}
}
