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

QT_TEST(drawablefilter_test)
{
	DRAWABLE_FILTER only2d, camtransfilter;
	only2d.SetFilter_is2d(true, true);
	camtransfilter.SetFilter_cameratransform(true, false);
	DRAWABLE d;
	d.Set2D(true);
	QT_CHECK(only2d.Matches(d));
	QT_CHECK(!camtransfilter.Matches(d));
}

SCENENODE & SCENENODE::AddNode()
{
	{
		SCENENODE newnode;
		childlist.push_back(newnode);
	}
	childlist.back().SetParent(*this);
	return childlist.back();
}

DRAWABLE & SCENENODE::AddDrawable()
{
	{
		DRAWABLE newnode;
		drawlist.push_back(newnode);
	}
	drawlist.back().SetParent(*this);
	return drawlist.back();
}

void SCENENODE::Delete(SCENENODE * todelete)
{
	list <SCENENODE>::iterator itdel = childlist.end();
	for (list <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		if (&(*i) == todelete)
			itdel = i;
	}
	
	assert(itdel != childlist.end()); //ensure parent contains child
	
	itdel->Clear(); //optional
	childlist.erase(itdel);
}

void SCENENODE::Delete(DRAWABLE * todelete)
{
	list <DRAWABLE>::iterator itdel = drawlist.end();
	for (list <DRAWABLE>::iterator i = drawlist.begin(); i != drawlist.end(); ++i)
	{
		if (&(*i) == todelete)
			itdel = i;
	}
	
	//ensure parent contains child
	assert(itdel != drawlist.end());
	
	drawlist.erase(itdel);
}

bool DRAWABLE_FILTER::Matches(const DRAWABLE & drawable) const
{
	return (drawable.filterspeedup.filtervalue & filtermask) == (filtervalue & filtermask);
}

class DRAWLISTCOPY_FUNCTOR
{
	private:
		const DRAWABLE_FILTER & filter;
		const MATRIX4 <float> & mat;
		vector <SCENEDRAW> & outvec;
		
	public:
		DRAWLISTCOPY_FUNCTOR(const DRAWABLE_FILTER & newfilter, const MATRIX4 <float> & newmat,
				     vector <SCENEDRAW> & output) : filter(newfilter),mat(newmat),
				     outvec(output) {}
		
		void operator() (const DRAWABLE & draw) const
		{
			if (filter.Matches(draw) && draw.GetDrawEnable())
			{
				outvec.push_back(SCENEDRAW(draw, mat));
			}
		}
};

//unsigned int SCENENODE::collapsed = 0;

void SCENENODE::GetCollapsedDrawList(map < DRAWABLE_FILTER *, vector <SCENEDRAW> > & drawlist_output_map, const MAT4 & prev_transform) const
{
	if (!active || (drawlist.empty() && childlist.empty())) return;
	
	MAT4 this_transform(prev_transform);
	
	bool identitytransform = transform.IsIdentityTransform();
	if (!identitytransform)
	{
		transform.GetRotation().GetMatrix4(this_transform);
		this_transform.Translate(transform.GetTranslation()[0], transform.GetTranslation()[1], transform.GetTranslation()[2]);
		this_transform = this_transform.Multiply(prev_transform);
	}
	
	for (map <DRAWABLE_FILTER *, vector <SCENEDRAW> >::iterator mi = drawlist_output_map.begin(); mi != drawlist_output_map.end(); ++mi)
	{
		for (list <DRAWABLE>::const_iterator i = drawlist.begin(); i != drawlist.end(); ++i)
		{
			if (i->GetDrawEnable())
			{
				if (mi->first->Matches(*i))
				{
					mi->second.push_back(SCENEDRAW(*i, this_transform));
				}
			}
		}
	}
	
	for (list <SCENENODE>::const_iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->GetCollapsedDrawList(drawlist_output_map, this_transform);
	}

	//collapsed++;
}

MAT4 SCENENODE::CollapseTransform() const
{
	list <const SCENENODE *> nodelist;
	const SCENENODE * curnode = this;
	
	while (curnode != NULL)
	{
		nodelist.push_back(this);
		curnode = curnode->parent;
	}
	
	nodelist.reverse();
	
	MAT4 outmat;
	for (list <const SCENENODE*>::iterator i = nodelist.begin(); i != nodelist.end(); ++i)
	{
		MAT4 rotmat;
		//rotmat.Set((*i)->transform.GetRotation());
		(*i)->transform.GetRotation().GetMatrix4(rotmat);
		rotmat.Translate((*i)->transform.GetTranslation()[0],(*i)->transform.GetTranslation()[1],(*i)->transform.GetTranslation()[2]);
		outmat = rotmat.Multiply(outmat);
	}
	
	return outmat;
}

VEC3 SCENENODE::TransformIntoWorldSpace(const VEC3 & localspace) const
{
	const SCENENODE * curnode = this;
	
	VEC3 pos(localspace);
	
	while (curnode != NULL)
	{
		curnode->transform.GetRotation().RotateVector(pos);
		pos = pos + curnode->transform.GetTranslation();
		
		curnode = curnode->parent;
	}
	
	return pos;
}

VEC3 SCENENODE::TransformIntoLocalSpace(const VEC3 & worldspace) const
{
	VEC3 pos(worldspace);
	
	list <const SCENENODE *> nodelist;
	const SCENENODE * curnode = this;
	
	while (curnode != NULL)
	{
		nodelist.push_back(curnode);
		curnode = curnode->parent;
	}
	
	nodelist.reverse();
	
	//cout << "Started at ";worldspace.DebugPrint();
	
	for (list <const SCENENODE*>::iterator i = nodelist.begin(); i != nodelist.end(); ++i)
	{
		//cout << "Subtracting ";(*i)->transform.GetTranslation().DebugPrint();
		pos = pos - (*i)->transform.GetTranslation();
		//cout << "Rotating" << endl;
		(-((*i)->transform.GetRotation())).RotateVector(pos);
	}
	
	return pos;
}

bool DRAWABLE::operator< (const DRAWABLE & other) const
{
	return (draw_order != other.draw_order ? (draw_order < other.draw_order) : false);
}

bool SCENEDRAW::operator< (const SCENEDRAW & other) const
{
	assert(draw);
	assert(other.draw);
	return ((*draw) < (*other.draw));
}

DRAWABLE::~DRAWABLE()
{

}

SCENENODE::~SCENENODE()
{

}

void SCENENODE::DebugPrint(std::ostream & out)
{
	out << "Drawables: " << drawlist.size() << endl;
	out << "Children: " << childlist.size() << endl;
	int count = 1;
	for (list <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		out << "---start child " << count << "---" << endl;
		i->DebugPrint(out);
		out << "+++end child " << count << "+++" << endl;
		
		count++;
	}
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

void SCENENODE::SortDrawablesByRenderState()
{
	drawlist.sort(DrawableSort_RenderState);
}

void SCENENODE::SetChildVisibility(bool newvis)
{
	for (list <DRAWABLE>::iterator i = drawlist.begin(); i != drawlist.end(); ++i)
	{
		i->SetDrawEnable(newvis);
	}
	
	for (list <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildVisibility(newvis);
	}
}

void SCENENODE::SetChildAlpha(float a)
{
	for (list <DRAWABLE>::iterator i = drawlist.begin(); i != drawlist.end(); ++i)
	{
		i->SetAlpha(a);
	}
	
	for (list <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildAlpha(a);
	}
}
