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

#ifndef _BEZIER_H
#define _BEZIER_H

#include "mathvector.h"
#include "aabb.h"

#include <fstream>

class Track;
class RoadPatch;

class Bezier
{
friend class Track;
friend class RoadPatch;

public:
	Bezier();
	~Bezier();
	Bezier(const Bezier & other) {CopyFrom(other);}
	Bezier & operator=(const Bezier & other) {return CopyFrom(other);}
	Bezier & CopyFrom(const Bezier &other);

	float GetDistFromStart() const {return dist_from_start;}
	void ResetDistFromStart() {dist_from_start = 0.0f;}
	void ResetNextPatch() {next_patch = NULL;}

	///initialize this bezier to the quad defined by the given corner points
	void SetFromCorners(const Vec3 & fl, const Vec3 & fr, const Vec3 & bl, const Vec3 & br);

	///shortest cubic spline through 4 on-curve points(chord approximation)
	///will modify point[1] and point[2] if fit possible
	void FitSpline(Vec3 p[]);

	///shortest cubic spline through 3 on-curve points(p1 == p2)
	///will modify point[1] and point[2]
	void FitMidPoint(Vec3 p[]);

	///attach this bezier and the other bezier by moving them and adjusting control points as necessary.
	/// note that the other patch will be modified
	void Attach(Bezier & other, bool reverse);
	void Attach(Bezier & other) {Attach(other, false);}

	///return true if the ray starting at the given origin going in the given direction intersects this bezier.
	/// output the contact point and normal to the given outtri and normal variables.
	bool CollideSubDivQuadSimple(const Vec3 & origin, const Vec3 & direction, Vec3 &outtri) const;
	bool CollideSubDivQuadSimpleNorm(const Vec3 & origin, const Vec3 & direction, Vec3 &outtri, Vec3 & normal) const;

	///read/write IO operations (ascii format)
	void ReadFrom(std::istream & openfile);
	void ReadFromYZX(std::istream & openfile);
	void WriteTo(std::ostream & openfile) const;

	///flip points on both axes
	void Reverse();

	///a diagnostic function that checks for a twisted bezier.  returns true if there is a problem.
	bool CheckForProblems() const;

	///halve the bezier defined by the given size 4 points4 array into the output size 4 arrays left4 and right4
	void DeCasteljauHalveCurve(Vec3 * points4, Vec3 * left4, Vec3 * right4) const;

	///access corners of the patch (front left, front right, back left, back right)
	const Vec3 & GetFL() const {return points[0][0];}
	const Vec3 & GetFR() const {return points[0][3];}
	const Vec3 & GetBL() const {return points[3][0];}
	const Vec3 & GetBR() const {return points[3][3];}

	///get the AABB that encloses this BEZIER
	Aabb <float> GetAABB() const;

	///access the bezier points where x = n % 4 and y = n / 4
	const Vec3 & operator[](const int n) const
	{
		assert(n < 16);
		int x = n % 4;
		int y = n / 4;
		return points[x][y];
	}

	const Vec3 & GetPoint(const unsigned int x, const unsigned int y) const
	{
		assert(x < 4);
		assert(y < 4);
		return points[x][y];
	}

	///return the 3D point on the bezier surface at the given normalized coordinates px and py
	Vec3 SurfCoord(float px, float py) const;

	///return the normal of the bezier surface at the given normalized coordinates px and py
	Vec3 SurfNorm(float px, float py) const;

	Bezier* GetNextPatch() const
	{
		return next_patch;
	}

	Vec3 GetRacingLine() const
	{
		return racing_line;
	}

	float GetTrackRadius() const
	{
		return track_radius;
	}

	bool HasRacingline() const
	{
		return have_racingline;
	}

private:
	///return the bernstein given the normalized coordinate u (zero to one) and an array of four points p
	Vec3 Bernstein(float u, const Vec3 p[]) const;

	///return the bernstein tangent given the normalized coordinate u (zero to one) and an array of four points p
	Vec3 BernsteinTangent(float u, const Vec3 p[]) const;

	///return true if the ray at orig with direction dir intersects the given quadrilateral.
	/// also put the collision depth in t and the collision coordinates in u,v
	bool IntersectQuadrilateralF(
		const Vec3 & orig,
		const Vec3 & dir,
		const Vec3 & v_00,
		const Vec3 & v_10,
		const Vec3 & v_11,
		const Vec3 & v_01,
		float &t, float &u, float &v) const;

	Vec3 points[4][4];
	Vec3 center;
	float radius;
	float length;
	float dist_from_start;

	Bezier *next_patch;
	float track_radius;
	int turn; //-1 - this is a left turn, +1 - a right turn, 0 - straight
	float track_curvature;
	Vec3 racing_line;
	bool have_racingline;
};

std::ostream & operator << (std::ostream &os, const Bezier & b);

#endif
