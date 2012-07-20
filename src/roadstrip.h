#ifndef _ROADSTRIP_H
#define _ROADSTRIP_H

#include "roadpatch.h"
#include "aabb_space_partitioning.h"
#include "optional.h"

class ROADSTRIP
{
public:
	ROADSTRIP();

	bool ReadFrom(
		std::istream & openfile,
		bool reverse,
		std::ostream & error_output);

	bool Collide(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		const float seglen,
		int & patch_id,
		MATHVECTOR <float, 3> & outtri,
		const BEZIER * & colpatch,
		MATHVECTOR <float, 3> & normal) const;

	void CreateRacingLine(
		SCENENODE & parentnode,
		std::tr1::shared_ptr<TEXTURE> racingline_texture);

	const std::vector<ROADPATCH> & GetPatches() const
	{
		return patches;
	}

	std::vector<ROADPATCH> & GetPatches()
	{
		return patches;
	}

	bool GetClosed() const
	{
		return closed;
	}

private:
	std::vector<ROADPATCH> patches;
	AABB_SPACE_PARTITIONING_NODE <unsigned> aabb_part;
	bool closed;

	void GenerateSpacePartitioning();
};

#endif // _ROADSTRIP_H
