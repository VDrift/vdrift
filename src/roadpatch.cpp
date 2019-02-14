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

#include "roadpatch.h"

#include <cmath>

RoadPatch::RoadPatch():
	next(NULL),
	track_radius(0),
	track_curvature(0),
	length(0),
	dist_from_start(0),
	have_racingline(false)
{
	// ctor
}

void RoadPatch::Attach(RoadPatch & other, bool reverse)
{
	//CheckForProblems();

	// store the pointer to next patch
	next = &other;

	// calculate the track radius at the connection of this patch and next patch
	Vec3 a = SurfCoord(0.5,0.0);
	Vec3 b = SurfCoord(0.5,1.0);
	Vec3 c = other.SurfCoord(0.5,1.0);

	if (reverse)
	{
		a = SurfCoord(0.5,1.0);
		b = SurfCoord(0.5,0.0);
		c = other.SurfCoord(0.5,0.0);

		//Reverse();
	}

	//racing_line = a;
	Vec3 d1 = a - b;
	Vec3 d2 = c - b;
	float d1mag = d1.Magnitude();
	float d2mag = d2.Magnitude();
	float diff = d2mag - d1mag;
	float dd = ((d1mag < 1E-4f) || (d2mag < 1E-4f)) ? 0 : d1.Normalize().dot(d2.Normalize());
	float angle = std::acos((dd >= 1) ? 1 : (dd <= -1) ? -1 : dd);
	float d1d2mag = d1mag + d2mag;
	float alpha = (d1d2mag < 1E-4f) ? 0 : (float(M_PI) * diff + 2 * d1mag * angle) / d1d2mag * 0.5f;
	if (std::abs(alpha - float(M_PI_2)) < 1E-3f)
		track_radius = 10000;
	else
		track_radius = 0.5f * d1mag / std::cos(alpha);

	length = d1mag;

	// calculate distance from start of the road
	if (other.next == NULL || reverse)
		other.dist_from_start = dist_from_start + length;
}

void RoadPatch::CalculateDistanceFromStart()
{
	dist_from_start = 0;
	float total_dist = length;
	auto patch = next;
	while (patch && patch != this)
	{
		patch->dist_from_start = total_dist;
		total_dist += patch->length;
		patch = patch->next;
	}
}

bool RoadPatch::Collide(
	const Vec3 & origin,
	const Vec3 & direction,
	float seglen,
	Vec3 & outtri,
	Vec3 & normal) const
{
	bool col = CollideSubDivQuadSimpleNorm(origin, direction, outtri, normal);
	float len = (outtri - origin).Magnitude();
	return col && len <= seglen;
}
