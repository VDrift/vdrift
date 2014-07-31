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

#include "roadstrip.h"
#include <algorithm>

RoadStrip::RoadStrip() :
	closed(false)
{
	// ctor
}

bool RoadStrip::ReadFrom(
	std::istream & openfile,
	bool reverse,
	std::ostream & error_output)
{
	assert(openfile);

	int num = 0;
	openfile >> num;

	patches.clear();
	patches.reserve(num);

	// Add all road patches to this strip.
	int badcount = 0;
	for (int i = 0; i < num; ++i)
	{
		Bezier * prevbezier = 0;
		if (!patches.empty()) prevbezier = &patches.back().GetPatch();

		patches.push_back(RoadPatch());
		patches.back().GetPatch().ReadFromYZX(openfile);

		if (prevbezier) prevbezier->Attach(patches.back().GetPatch());

		if (patches.back().GetPatch().CheckForProblems())
		{
			badcount++;
			patches.pop_back();
		}
	}

	if (badcount > 0)
		error_output << "Rejected " << badcount << " bezier patch(es) from roadstrip due to errors" << std::endl;

	// Reverse patches.
	if (reverse)
	{
		std::reverse(patches.begin(), patches.end());
		for (std::vector<RoadPatch>::iterator i = patches.begin(); i != patches.end(); ++i)
		{
			i->GetPatch().Reverse();
		}
	}

	// Close the roadstrip if it ends near where it starts.
	closed = (patches.size() > 2) &&
		((patches.back().GetPatch().GetFL() - patches.front().GetPatch().GetBL()).Magnitude() < 0.1) &&
		((patches.back().GetPatch().GetFR() - patches.front().GetPatch().GetBR()).Magnitude() < 0.1);

	// Connect patches.
	for (std::vector<RoadPatch>::iterator i = patches.begin(); i != patches.end() - 1; ++i)
	{
		std::vector<RoadPatch>::iterator n = i + 1;
		i->GetPatch().Attach(n->GetPatch());
	}
	if (closed)
	{
		patches.back().GetPatch().Attach(patches.front().GetPatch());
	}

	GenerateSpacePartitioning();

	return true;
}

void RoadStrip::GenerateSpacePartitioning()
{
	aabb_part.Clear();
	for (unsigned i = 0; i < patches.size(); ++i)
	{
		aabb_part.Add(i, patches[i].GetPatch().GetAABB());
	}
	aabb_part.Optimize();
}

bool RoadStrip::Collide(
	const Vec3 & origin,
	const Vec3 & direction,
	const float seglen,
	int & patch_id,
	Vec3 & outtri,
	const Bezier * & colpatch,
	Vec3 & normal) const
{
	if (patch_id >= 0 && patch_id < (int)patches.size())
	{
		Vec3 coltri, colnorm;
		if (patches[patch_id].Collide(origin, direction, seglen, coltri, colnorm))
		{
			outtri = coltri;
			normal = colnorm;
			colpatch = &patches[patch_id].GetPatch();
			return true;
		}
	}

	bool col = false;
	std::vector<int> candidates;
	aabb_part.Query(Aabb<float>::Ray(origin, direction, seglen), candidates);
	for (std::vector<int>::iterator i = candidates.begin(); i != candidates.end(); ++i)
	{
		Vec3 coltri, colnorm;
		if (patches[*i].Collide(origin, direction, seglen, coltri, colnorm))
		{
			if (!col || (coltri-origin).MagnitudeSquared() < (outtri-origin).MagnitudeSquared())
			{
				outtri = coltri;
				normal = colnorm;
				colpatch = &patches[*i].GetPatch();
				patch_id = *i;
			}
			col = true;
		}
	}

	return col;
}
