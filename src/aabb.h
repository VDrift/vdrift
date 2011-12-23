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
#include "mathplane.h"
#include "frustum.h"
#include <ostream>

template <typename T>
class AABB
{
public:
	AABB();
	AABB(const AABB <T> & other);

	const AABB <T> & operator = (const AABB <T> & other);

	bool operator == (const AABB<T> & other) const;

	/// WARNING: Not implemented.
	bool operator < (const AABB<T> & other) const;

	const MATHVECTOR <T, 3> & GetPos() const;
	const MATHVECTOR <T, 3> & GetSize() const;
	const MATHVECTOR <T, 3> & GetCenter() const;

	void DebugPrint(std::ostream & o) const;

	void DebugPrint2(std::ostream & o) const;

	void SetFromSphere(const MATHVECTOR <T, 3> & newcenter, float newRadius);

	void SetFromCorners(const MATHVECTOR <T, 3> & c1, const MATHVECTOR <T, 3> & c2);

	void CombineWith(const AABB <T> & other);

	// For intersection test returns.
	enum INTERSECTION
	{
		OUT,
		INTERSECT,
		IN
	};

	struct INTERSECT_ALWAYS{};

	class RAY
	{
	public:
		RAY(const MATHVECTOR <T, 3> & neworig, const MATHVECTOR <T, 3> & newdir, T newseglen);

		MATHVECTOR <T, 3> orig;
		MATHVECTOR <T, 3> dir;
		T seglen;
	};

	INTERSECTION Intersect(const RAY & ray) const;

	INTERSECTION Intersect(const AABB <T> & other) const;

	INTERSECTION Intersect(const FRUSTUM & frustum) const;

	INTERSECTION Intersect(INTERSECT_ALWAYS always) const;

private:
	/// Minimum corner (center-size*0.5).
	MATHVECTOR <T, 3> pos;
	/// Exact center of AABB.
	MATHVECTOR <T, 3> center;
	/// Size of AABB.
	MATHVECTOR <T, 3> size;
	/// Radius (size.Magnitude()*0.5).
	float radius;

	void recomputeRadius();
};

#endif
