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

#ifndef _ROADSTRIP_H
#define _ROADSTRIP_H

#include "roadpatch.h"
#include "aabbtree.h"

#include <iosfwd>
#include <vector>

class RoadStrip
{
public:
	RoadStrip();

	bool ReadFrom(
		std::istream & openfile,
		bool reverse,
		std::ostream & error_output);

	bool Collide(
		const Vec3 & origin,
		const Vec3 & direction,
		const float seglen,
		int & patch_id,
		Vec3 & outtri,
		const Bezier * & colpatch,
		Vec3 & normal) const;

	const std::vector<RoadPatch> & GetPatches() const
	{
		return patches;
	}

	std::vector<RoadPatch> & GetPatches()
	{
		return patches;
	}

	bool GetClosed() const
	{
		return closed;
	}

private:
	std::vector<RoadPatch> patches;
	AabbTreeNode <unsigned> aabb_part;
	bool closed;

	void GenerateSpacePartitioning();
};

#endif // _ROADSTRIP_H
