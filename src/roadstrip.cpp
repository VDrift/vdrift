#include "roadstrip.h"
#include <algorithm>

bool ROADSTRIP::ReadFrom(std::istream & openfile, std::ostream & error_output)
{
	assert(openfile);

	//number of patches in this road strip
	int num = 0;
	openfile >> num;

	patches.clear();
	patches.reserve(num);

	//add all road patches to this strip
	int badcount = 0;
	for (int i = 0; i < num; ++i)
	{
		BEZIER * prevbezier = 0;
		if (!patches.empty()) prevbezier = &patches.back().GetPatch();

		patches.push_back(ROADPATCH());
		patches.back().GetPatch().ReadFrom(openfile);

		if (prevbezier) prevbezier->Attach(patches.back().GetPatch());

		if (patches.back().GetPatch().CheckForProblems())
		{
			badcount++;
			patches.pop_back();
		}
	}

	if (badcount > 0)
		error_output << "Rejected " << badcount << " bezier patch(es) from roadstrip due to errors" << std::endl;

	//close the roadstrip
	if (patches.size() > 2)
	{
		//only close it if it ends near where it starts
		if (((patches.back().GetPatch().GetFL() - patches.front().GetPatch().GetBL()).Magnitude() < 0.1) &&
		    ((patches.back().GetPatch().GetFR() - patches.front().GetPatch().GetBR()).Magnitude() < 0.1))
		{
			patches.back().GetPatch().Attach(patches.front().GetPatch());
			closed = true;
		}
	}

	GenerateSpacePartitioning();

	return true;
}

void ROADSTRIP::GenerateSpacePartitioning()
{
	aabb_part.Clear();
	for (int i = 0; i < (int)patches.size(); ++i)
	{
		aabb_part.Add(i, patches[i].GetPatch().GetAABB());
	}
	aabb_part.Optimize();
}

bool ROADSTRIP::Collide(
	const MATHVECTOR <float, 3> & origin,
	const MATHVECTOR <float, 3> & direction,
	const float seglen,
	int & patch_id,
	MATHVECTOR <float, 3> & outtri,
	const BEZIER * & colpatch,
	MATHVECTOR <float, 3> & normal) const
{
	if (patch_id >= 0 && patch_id < (int)patches.size())
	{
		MATHVECTOR <float, 3> coltri, colnorm;
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
	aabb_part.Query(AABB<float>::RAY(origin, direction, seglen), candidates);
	for (std::vector<int>::iterator i = candidates.begin(); i != candidates.end(); ++i)
	{
		MATHVECTOR <float, 3> coltri, colnorm;
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

void ROADSTRIP::Reverse()
{
	std::reverse(patches.begin(), patches.end());

	for (std::vector<ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		i->GetPatch().Reverse();
		i->GetPatch().ResetDistFromStart();
	}

	//fix pointers to next patches for race placement
	for (std::vector<ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		std::vector<ROADPATCH>::iterator n = i;
		n++;
		BEZIER * nextpatchptr = 0;
		if (n != patches.end())
		{
			nextpatchptr = &(n->GetPatch());
			i->GetPatch().Attach(*nextpatchptr);
		}
		else
		{
			i->GetPatch().ResetNextPatch();
			i->GetPatch().Attach(patches.front().GetPatch());
		}
	}
}

void ROADSTRIP::CreateRacingLine(
	SCENENODE & parentnode,
	std::tr1::shared_ptr<TEXTURE> racingline_texture,
	std::ostream & error_output)
{
	for (std::vector<ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		std::vector<ROADPATCH>::iterator n = i;
		n++;
		ROADPATCH * nextpatch(0);
		if (n != patches.end())
		{
			nextpatch = &(*n);
		} else {
			nextpatch = &(*patches.begin());
		}
		i->AddRacinglineScenenode(parentnode, nextpatch, racingline_texture, error_output);
	}
}


