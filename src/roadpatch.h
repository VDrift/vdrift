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

#ifndef _ROADPATCH_H
#define _ROADPATCH_H

#include "bezier.h"

class RoadPatch : public Bezier
{
public:
	RoadPatch();

	/// attach this patch and the other by moving them and adjusting control points as necessary.
	/// note that the other patch will be modified
	void Attach(RoadPatch & other, bool reverse = false);

	/// calculate distance from this start patch
	/// note that attached patches will be modified
	void CalculateDistanceFromStart();

	void SetRacingLine(const Vec3 & position, float curvature)
	{
		racing_line = position;
		track_curvature = curvature;
		have_racingline = true;
	}

	/// return true if the ray starting at the given origin going in the given direction intersects this patch.
	/// output the contact point and normal to the given outtri and normal variables.
	bool Collide(
		const Vec3 & origin,
		const Vec3 & direction,
		float seglen,
		Vec3 & outtri,
		Vec3 & normal) const;

	RoadPatch * GetNextPatch() const
	{
		return next;
	}

	Vec3 GetRacingLine() const
	{
		return racing_line;
	}

	float GetTrackRadius() const
	{
		return track_radius;
	}

	float GetDistFromStart() const
	{
		return dist_from_start;
	}

	bool HasRacingline() const
	{
		return have_racingline;
	}

private:
	RoadPatch * next;
	Vec3 racing_line;
	float track_radius;
	float track_curvature;
	float length;
	float dist_from_start;
	bool have_racingline;
};

#endif // _ROADPATCH_H
