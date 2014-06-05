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

class RoadPatch
{
public:
	RoadPatch() : track_curvature(0) {}

	const Bezier & GetPatch() const {return patch;}

	Bezier & GetPatch() {return patch;}

	///return true if the ray starting at the given origin going in the given direction intersects this patch.
	/// output the contact point and normal to the given outtri and normal variables.
	bool Collide(
		const Vec3 & origin,
		const Vec3 & direction,
		float seglen,
		Vec3 & outtri,
		Vec3 & normal) const;

	float GetTrackCurvature() const
	{
		return track_curvature;
	}

	Vec3 GetRacingLine() const
	{
		return racing_line;
	}

	void SetTrackCurvature ( float value )
	{
		track_curvature = value;
	}

	void SetRacingLine ( const MathVector< float, 3 >& value )
	{
		racing_line = value;
		patch.racing_line = value;
		patch.have_racingline = true;
	}

private:
	Bezier patch;
	float track_curvature;
	Vec3 racing_line;
};

#endif // _ROADPATCH_H
