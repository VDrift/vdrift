#ifndef _ROADSTRIP_H
#define _ROADSTRIP_H

#include "roadpatch.h"
#include "aabb_space_partitioning.h"
#include "optional.h"

class ROADSTRIP
{
public:
	ROADSTRIP() : closed(false) {}

	bool ReadFrom(std::istream & openfile, std::ostream & error_output);

	bool Collide(
		const MATHVECTOR <float, 3> & origin,
		const MATHVECTOR <float, 3> & direction,
		const float seglen,
		int & patch_id,
		MATHVECTOR <float, 3> & outtri,
		const BEZIER * & colpatch,
		MATHVECTOR <float, 3> & normal) const;

	void Reverse();

	const std::vector<ROADPATCH> & GetPatches() const {return patches;}

	std::vector<ROADPATCH> & GetPatches() {return patches;}

	void CreateRacingLine(
		SCENENODE & parentnode,
		std::tr1::shared_ptr<TEXTURE> racingline_texture);

	bool GetClosed() const
	{
		return closed;
	}

	///either returns a const BEZIER * to the roadpatch at the given (positive or negative) offset from the supplied bezier (looping around if necessary) or does not return a value if the bezier is not found in this roadstrip.
	optional <const BEZIER *> FindBezierAtOffset(const BEZIER * bezier, int offset=0) const;

private:
	std::vector<ROADPATCH> patches;
	AABB_SPACE_PARTITIONING_NODE <unsigned> aabb_part;
	bool closed;

	void GenerateSpacePartitioning();
};

#endif // _ROADSTRIP_H
