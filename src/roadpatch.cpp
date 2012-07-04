#include "roadpatch.h"
#include "scenenode.h"

bool ROADPATCH::Collide(
	const MATHVECTOR <float, 3> & origin,
	const MATHVECTOR <float, 3> & direction,
	float seglen, MATHVECTOR <float, 3> & outtri,
	MATHVECTOR <float, 3> & normal) const
{
	bool col = patch.CollideSubDivQuadSimpleNorm(origin, direction, outtri, normal);
	float len = (outtri - origin).Magnitude();
	return col && len <= seglen;
}

void ROADPATCH::AddRacinglineScenenode(
	SCENENODE & node,
	ROADPATCH * nextpatch,
	std::tr1::shared_ptr<TEXTURE> racingline_texture)
{
	if (!nextpatch) return;

	//Create racing line scenenode
	keyed_container <DRAWABLE>::handle drawhandle = node.GetDrawlist().normal_blend.insert(DRAWABLE());
	DRAWABLE & draw = node.GetDrawlist().normal_blend.get(drawhandle);

	draw.SetDiffuseMap(racingline_texture);
	draw.SetDecal(true);
	draw.SetVertArray(&racingline_vertexarray);

	MATHVECTOR <float, 3> v0 = racing_line + (patch.GetPoint(0,0) - racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v1 = racing_line + (patch.GetPoint(0,3) - racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v2 = nextpatch->racing_line + (nextpatch->GetPatch().GetPoint(0,3) - nextpatch->racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v3 = nextpatch->racing_line + (nextpatch->GetPatch().GetPoint(0,0) - nextpatch->racing_line).Normalize()*0.1;

	//transform from bezier space into world space
	v0.Set(v0[2],v0[0],v0[1]);
	v1.Set(v1[2],v1[0],v1[1]);
	v2.Set(v2[2],v2[0],v2[1]);
	v3.Set(v3[2],v3[0],v3[1]);

	float trackoffset = 0.1;
	v0[2] += trackoffset;
	v1[2] += trackoffset;
	v2[2] += trackoffset;
	v3[2] += trackoffset;

	float vcorners[12];
	float uvs[8];
	int bfaces[6];

	//std::cout << v0 << std::endl;

	vcorners[0] = v0[0]; vcorners[1] = v0[1]; vcorners[2] = v0[2];
	vcorners[3] = v1[0]; vcorners[4] = v1[1]; vcorners[5] = v1[2];
	vcorners[6] = v2[0]; vcorners[7] = v2[1]; vcorners[8] = v2[2];
	vcorners[9] = v3[0]; vcorners[10] = v3[1]; vcorners[11] = v3[2];

	//std::cout << v0 << endl;
	//std::cout << racing_line << endl;

	uvs[0] = 0;
	uvs[1] = 0;
	uvs[2] = 1;
	uvs[3] = 0;
	uvs[4] = 1;
	uvs[5] = (v2-v1).Magnitude();
	uvs[6] = 0;
	uvs[7] = (v2-v1).Magnitude();

	bfaces[0] = 0;
	bfaces[1] = 2;
	bfaces[2] = 1;
	bfaces[3] = 0;
	bfaces[4] = 3;
	bfaces[5] = 2;

	racingline_vertexarray.SetFaces(bfaces, 6);
	racingline_vertexarray.SetVertices(vcorners, 12);
	racingline_vertexarray.SetTexCoordSets(1);
	racingline_vertexarray.SetTexCoords(0, uvs, 8);
}
