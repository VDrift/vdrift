#include "scenegraph.h"
#include "texture.h"
#include "containeralgorithm.h"
#include "unittest.h"

#include <string>
using std::string;

#include <list>
using std::list;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <iostream>
using std::endl;

typedef MATRIX4<float> MAT4;
typedef MATHVECTOR<float,3> VEC3;
typedef QUATERNION<float> QUAT;

void SCENENODE::Traverse(DRAWABLE_CONTAINER <PTRVECTOR> & drawlist_output, const MAT4 & prev_transform)
{
	if (drawlist.empty() && childlist.empty())
		return;
	
	MAT4 this_transform(prev_transform);
	
	bool identitytransform = transform.IsIdentityTransform();
	if (!identitytransform)
	{
		transform.GetRotation().GetMatrix4(this_transform);
		this_transform.Translate(transform.GetTranslation()[0], transform.GetTranslation()[1], transform.GetTranslation()[2]);
		this_transform = this_transform.Multiply(prev_transform);
	}
	
	if (this_transform != cached_transform)
		drawlist.AppendTo<DRAWABLE_CONTAINER<PTRVECTOR>,true>(drawlist_output, this_transform);
	else
		drawlist.AppendTo<DRAWABLE_CONTAINER<PTRVECTOR>,false>(drawlist_output, this_transform);
	
	for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->Traverse(drawlist_output, this_transform);
	}
	
	cached_transform = this_transform;
}

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

bool DRAWABLE::operator< (const DRAWABLE & other) const
{
	//if (draw_order != other.draw_order)
	//	return (draw_order < other.draw_order);
	
	/*if (diffuse_map.PtrValid() && other.diffuse_map.PtrValid())
		return (diffuse_map.GetPtr()->GetTexture().GetTextureInfo().GetName() < other.diffuse_map.GetPtr()->GetTexture().GetTextureInfo().GetName());*/
	
	//return false;
	
	return (draw_order != other.draw_order ? (draw_order < other.draw_order) : false);
}

bool DrawableSort_RenderState (DRAWABLE & first, DRAWABLE & second)
{
	bool b1,b2;
	
	b1 = first.GetPartialTransparency();
	b2 = second.GetPartialTransparency();
	if (b1 != b2)
		return (b1 < b2);
	
	b1 = first.GetSkybox();
	b2 = second.GetSkybox();
	if (b1 != b2)
		return (b1 < b2);
	
	b1 = first.GetLit();
	b2 = second.GetLit();
	if (b1 != b2)
		return (b1 < b2);
	
	b1 = first.GetDecal();
	b2 = second.GetDecal();
	if (b1 != b2)
		return (b1 < b2);
	
	//string dmap1(first.GetDiffuseMapName()), dmap2(second.GetDiffuseMapName());
	string dmap1, dmap2;
	if (first.GetDiffuseMap())
		dmap1 = first.GetDiffuseMap()->GetTextureInfo().GetName();
	if (second.GetDiffuseMap())
		dmap1 = second.GetDiffuseMap()->GetTextureInfo().GetName();
	if (dmap1 != dmap2)
		return (dmap1 < dmap2);
	
	return false;
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

void SCENENODE::DebugPrint(std::ostream & out, int curdepth)
{
	for (int i = 0; i < curdepth; i++)
		out << "-";
	out << "Children: " << Nodes() << ", Drawables: " << Drawables() << std::endl;
	
	for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->DebugPrint(out, curdepth+1);
	}
}
