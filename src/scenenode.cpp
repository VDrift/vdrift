#include "scenenode.h"

typedef MATRIX4<float> MAT4;
typedef MATHVECTOR<float,3> VEC3;
typedef QUATERNION<float> QUAT;

VEC3 SCENENODE::TransformIntoWorldSpace(const VEC3 & localspace) const
{
	VEC3 out(localspace);
	cached_transform.TransformVectorOut(out[0], out[1], out[2]);
	return out;
}

VEC3 SCENENODE::TransformIntoLocalSpace(const VEC3 & worldspace) const
{
	VEC3 out(worldspace);
	cached_transform.TransformVectorIn(out[0], out[1], out[2]);
	return out;
}

void SCENENODE::SetChildVisibility(bool newvis)
{
	drawlist.SetVisibility(newvis);

	for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildVisibility(newvis);
	}
}

void SCENENODE::SetChildAlpha(float a)
{
	drawlist.SetAlpha(a);

	for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildAlpha(a);
	}
}

void SCENENODE::DebugPrint(std::ostream & out, int curdepth) const
{
	for (int i = 0; i < curdepth; i++)
		out << "-";
	out << "Children: " << Nodes() << ", Drawables: " << Drawables() << std::endl;

	for (keyed_container <SCENENODE>::const_iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->DebugPrint(out, curdepth+1);
	}
}
