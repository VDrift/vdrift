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

#include <iostream>

class BEZIER
{
public:
	BEZIER();

	~BEZIER();

	BEZIER(const BEZIER & other);

	BEZIER & operator=(const BEZIER & other);

	BEZIER & CopyFrom(const BEZIER & other);

	/// initialize this bezier to the quad defined by the given corner points
	void SetFromCorners(
		const MATHVECTOR <float, 3> & fl,
		const MATHVECTOR <float, 3> & fr,
		const MATHVECTOR <float, 3> & bl,
		const MATHVECTOR <float, 3> & br);

	/// shortest cubic spline through 4 on-curve points(chord approximation)
	/// will modify point[1] and point[2] if fit possible
	void FitSpline(MATHVECTOR <float, 3> p[]);

	/// shortest cubic spline through 3 on-curve points(p1 == p2)
	/// will modify point[1] and point[2]
	void FitMidPoint(MATHVECTOR <float, 3> p[]);

	/// attach this bezier and the other bezier by moving them
	/// and adjusting control points as necessary.
	/// note that the other patch will be modified
	void Attach(BEZIER & other);

	/// set racing line, position as width fraction between BR an BL points
	void SetRacingLine(float width_fraction, float curvature);

	/// return true if the ray starting at the given origin going in the given direction intersects this bezier.
	/// output the contact point and normal to the given outtri and normal variables.
	bool CollideSubDivQuadSimpleNorm(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		MATHVECTOR <float, 3> & outtri,
		MATHVECTOR <float, 3> & normal) const;

	/// read/write IO operations (ascii format)
	void ReadFrom(std::istream & openfile);
	void WriteTo(std::ostream & openfile) const;

	/// flip points on both axes
	void Reverse();

	/// a diagnostic function that checks for a twisted bezier.  returns true if there is a problem.
	bool CheckForProblems() const;

	/// halve the bezier defined by the given size 4 points4 array into the output size 4 arrays left4 and right4
	void DeCasteljauHalveCurve(
		MATHVECTOR <float, 3> * points4,
		MATHVECTOR <float, 3> * left4,
		MATHVECTOR <float, 3> * right4) const;

	/// access corners of the patch (front left, front right, back left, back right)
	const MATHVECTOR <float, 3> & GetFL() const;
	const MATHVECTOR <float, 3> & GetFR() const;
	const MATHVECTOR <float, 3> & GetBL() const;
	const MATHVECTOR <float, 3> & GetBR() const;

	/// points derived from corners (not necessary on surface), direction not normalized
	MATHVECTOR <float, 3> GetFrontCenter() const;
	MATHVECTOR <float, 3> GetBackCenter() const;
	MATHVECTOR <float, 3> GetCenter() const;
	MATHVECTOR <float, 3> GetCenterWidth() const;
	MATHVECTOR <float, 3> GetCenterLine() const;

	/// access the bezier points where x = n % 4 and y = n / 4
	const MATHVECTOR <float, 3> & operator[](const int n) const;

	/// access the bezier points by their x, y indices
	const MATHVECTOR <float, 3> & GetPoint(unsigned x, unsigned y) const;

	/// get the AABB that encloses this BEZIER
	AABB <float> GetAABB() const;

	/// return the 3D point on the bezier surface at the given normalized coordinates px and py
	MATHVECTOR <float, 3> SurfPoint(float px, float py) const;

	/// return the normal of the bezier surface at the given normalized coordinates px and py
	MATHVECTOR <float, 3> SurfNorm(float px, float py) const;

	/// get total distance from first patch
	/// for a closed track the first patch contains total track length
	float GetDistFromStart() const;

	/// get next patch, zero if no followup patches
	const BEZIER * GetNextPatch() const;

	/// get next closest patch from point
	const BEZIER * GetNextClosestPatch(const MATHVECTOR<float, 3> & point) const;

	/// get patch width at back
	float GetWidth() const;

	/// get patch center line length
	float GetLength() const;

	/// get racing line point (at patch back)
	/// if racingline not set return back center
	const MATHVECTOR <float, 3> & GetRacingLine() const;

	/// get racing line width fraction: p = bl * (1 - fraction) + br * fraction
	/// if racing line not set return 0.5
	float GetRacingLineFraction() const;

	/// get 1 / racing_line_curvature
	/// if racing line not set return back center radius
	float GetRacingLineRadius() const;

	/// iterate trough patches, recalculate distance from start
	static void CalculateDistFromStart(BEZIER & start_patch);

private:
	MATHVECTOR <float, 3> points[4][4];	///< 16 patch control points
	float length;						///< patch length along center line
	float width;						///< patch width at back
	float dist_from_start;				///< accumulated length
	BEZIER * next_patch;				///< attached patch

	MATHVECTOR <float, 3> racing_line;	///< racing line point
	float racing_line_fraction;			///< racing line width fraction
	float racing_line_radius;			///< 1 / racing line curvature

	/// return the bernstein given the normalized coordinate u (zero to one) and an array of four points p
	MATHVECTOR <float, 3> Bernstein(float u, const MATHVECTOR <float, 3> p[]) const;

	/// return the bernstein tangent given the normalized coordinate u (zero to one) and an array of four points p
	MATHVECTOR <float, 3> BernsteinTangent(float u, const MATHVECTOR <float, 3> p[]) const;

	/// return true if the ray at orig with direction dir intersects the given quadrilateral.
	/// also put the collision depth in t and the collision coordinates in u,v
	bool IntersectQuadrilateralF(
		const MATHVECTOR <float, 3> & orig,
		const MATHVECTOR <float, 3> & dir,
		const MATHVECTOR <float, 3> & v_00,
		const MATHVECTOR <float, 3> & v_10,
		const MATHVECTOR <float, 3> & v_11,
		const MATHVECTOR <float, 3> & v_01,
		float &t, float &u, float &v) const;
};

std::ostream & operator << (std::ostream & os, const BEZIER & b);

// implementation

inline void BEZIER::SetRacingLine(float width_fraction, float curvature)
{
	racing_line = GetBL() * (1.0 - width_fraction) + GetBR() * width_fraction;
	racing_line_fraction = width_fraction;
	racing_line_radius = (fabs(curvature) > 1E-4) ? 1 / curvature : 1E4;
}

inline const MATHVECTOR <float, 3> & BEZIER::GetFL() const
{
	return points[0][0];
}

inline const MATHVECTOR <float, 3> & BEZIER::GetFR() const
{
	return points[0][3];
}

inline const MATHVECTOR <float, 3> & BEZIER::GetBL() const
{
	return points[3][0];
}

inline const MATHVECTOR <float, 3> & BEZIER::GetBR() const
{
	return points[3][3];
}

inline MATHVECTOR <float, 3> BEZIER::GetFrontCenter() const
{
	return (GetFL() + GetFR()) * 0.5;
}

inline MATHVECTOR <float, 3> BEZIER::GetBackCenter() const
{
	return (GetBL() + GetBR()) * 0.5;
}

inline MATHVECTOR <float, 3> BEZIER::GetCenter() const
{
	return (GetFrontCenter() + GetBackCenter()) * 0.5;
}

inline MATHVECTOR <float, 3> BEZIER::GetCenterWidth() const
{
	return (GetFL() - GetFR() + GetBL() - GetBR()) * 0.5;
}

inline MATHVECTOR <float, 3> BEZIER::GetCenterLine() const
{
	return (GetFrontCenter() - GetBackCenter()) * 0.5;
}

inline const MATHVECTOR <float, 3> & BEZIER::operator[](const int n) const
{
	assert(n < 16);
	int x = n % 4;
	int y = n / 4;
	return points[x][y];
}

inline const MATHVECTOR <float, 3> & BEZIER::GetPoint(unsigned x, unsigned y) const
{
	assert(x < 4);
	assert(y < 4);
	return points[x][y];
}

inline float BEZIER::GetDistFromStart() const
{
	return dist_from_start;
}

inline const BEZIER * BEZIER::GetNextPatch() const
{
	return next_patch;
}

inline float BEZIER::GetWidth() const
{
	return width;
}

inline float BEZIER::GetLength() const
{
	return length;
}

inline const MATHVECTOR <float, 3> & BEZIER::GetRacingLine() const
{
	return racing_line;
}

inline float BEZIER::GetRacingLineFraction() const
{
	return racing_line_fraction;
}

inline float BEZIER::GetRacingLineRadius() const
{
	return racing_line_radius;
}

#endif
