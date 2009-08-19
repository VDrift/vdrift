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

/*const TESTER SCENEGRAPH::Test()
{
	TESTER mytest;
	
	int points = 0;
	
	if (GetRoot().Nodes() == 0) points++;
	SCENENODE & n1 = GetRoot().AddNode();
	if (GetRoot().Nodes() == 1) points++;
	SCENENODE & n2 = GetRoot().AddNode();
	if (GetRoot().Nodes() == 2) points++;
	SCENENODE & n21 = n2.AddNode();
	if (n2.Nodes() == 1) points++;
	n21.AddDrawable();
	if (n21.Drawables() == 1) points++;
	if (n2.Drawables() == 0) points++;
	mytest.SubTestComplete("Scene node and drawable management", points, 6);
	
	points = 0;
	DRAWABLE & d11 = n1.AddDrawable();
	Delete(&d11);
	if (n1.Drawables() == 0) points++;
	Delete(&n1);
	if (GetRoot().Nodes() == 1) points++;
	Delete(&n2);
	DRAWABLE & d3 = GetRoot().AddDrawable();
	if (GetRoot().Drawables() == 1) points++;
	Delete(&d3);
	if (GetRoot().Nodes() == 0 && GetRoot().Drawables() == 0) points++;
	mytest.SubTestComplete("Scene node and drawable deletion", points, 4);
	
	points = 0;
	DRAWABLE_FILTER filter;
	filter.SetFilter_is2d(false, false);
	DRAWABLE & d4 = GetRoot().AddDrawable();
	d4.Set2D(false);
	if (filter.Matches(d4)) points++;
	d4.Set2D(true);
	if (filter.Matches(d4)) points++;
	filter.SetFilter_is2d(true, false);
	if (!filter.Matches(d4)) points++;
	d4.Set2D(false);
	if (filter.Matches(d4)) points++;
	mytest.SubTestComplete("Drawable filtering", points, 4);
	
	points = 0;
	{
		SCENENODE n1;
		n1.GetTransform().SetTranslation(VEC3(0,2,0));
		QUAT r;
		r.SetAxisAngle(-3.141593*0.5, 0, 0, -1);
		n1.GetTransform().SetRotation(r);
		SCENENODE & n2 = n1.AddNode();
		n2.GetTransform().SetTranslation(VEC3(0,1,0));
		r.SetAxisAngle(3.141593*0.5, 0, 0, -1);
		n2.GetTransform().SetRotation(r);
		VEC3 v1(0.1,0,0);
		VEC3 v2(0.1,0,0);
		v1 = n2.TransformIntoWorldSpace(v1);
		v2 = n2.TransformIntoLocalSpace(v2);
		if ((v1 - VEC3(-0.9,2,0)).len() < 0.001 && (v2 - VEC3(1.1,-2,0)).len() < 0.001) points++;
	}
	mytest.SubTestComplete("Transform into and out of local space", points, 1);

	return mytest;
}*/

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

/*bool DRAWABLE_FILTER::Matches(const DRAWABLE & drawable) const
{
	if (is2d_filter && is2d != drawable.is2d) return false;
	if (partial_transparency_filter && partial_transparency != drawable.partial_transparency) return false;
	if (skybox_filter && skybox != drawable.skybox) return false;
	if (blur_filter && blur != drawable.blur) return false;
	if (cameratransform_filter && cameratransform != drawable.cameratransform) return false;
	
	return true;
}*/

bool DRAWABLE_FILTER::Matches(const DRAWABLE & drawable) const
{
	return (drawable.filterspeedup.filtervalue & filtermask) == (filtervalue & filtermask);
}

/*void SCENEGRAPH::GetDrawList(list <DRAWABLE_FILTER *> & filter_list, map < DRAWABLE_FILTER *, list <SCENEDRAW> > & drawlist_output_map) const
{
	drawlist_output_map.clear();
	
	ITERATE(DRAWABLE_FILTER *,filter_list,i)
	{
		list <SCENEDRAW> & outlist = drawlist_output_map[*i];
		rootnode.GetDrawList( outlist, **i);
		//cout << drawlist_output_map[*i].size() << endl;
	}
}

void SCENEGRAPH::GetCollapsedDrawList(list <DRAWABLE_FILTER *> & filter_list, map < DRAWABLE_FILTER *, list <SCENEDRAW> > & drawlist_output_map) const
{
	drawlist_output_map.clear();
	
	ITERATE(DRAWABLE_FILTER *,filter_list,i)
	{
		drawlist_output_map[*i].clear(); //this creates the entry in our map
	}
	
	MAT4 identity;
	
	rootnode.GetCollapsedDrawList(filter_list, drawlist_output_map, identity);
}*/

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

void SCENENODE::GetCollapsedDrawList(map < DRAWABLE_FILTER *, vector <SCENEDRAW> > & drawlist_output_map, const MAT4 & prev_transform) const
{
	if (drawlist.empty() && childlist.empty())
		return;
	
	MAT4 this_transform(prev_transform);
	
	bool identitytransform = transform.IsIdentityTransform();
	if (!identitytransform)
	{
		//MAT4 curmat;
		//curmat.Set(this_transform);
		//MAT4 transmat;
		//transmat.Translate(transform.GetTranslation());
		//MAT4 rotmat;
		//rotmat.Set(transform.GetRotation());
		transform.GetRotation().GetMatrix4(this_transform);
		this_transform.Translate(transform.GetTranslation()[0], transform.GetTranslation()[1], transform.GetTranslation()[2]);
		
		//this_transform = (rotmat.Multiply(transmat)).Multiply(curmat);
		this_transform = this_transform.Multiply(prev_transform);
	}
	
	for (map <DRAWABLE_FILTER *, vector <SCENEDRAW> >::iterator mi = drawlist_output_map.begin(); mi != drawlist_output_map.end(); ++mi)
	{
		//calgo::for_each(drawlist, DRAWLISTCOPY_FUNCTOR(*mi->first, this_transform, mi->second));
		
		for (list <DRAWABLE>::const_iterator i = drawlist.begin(); i != drawlist.end(); ++i)
		{
			if (mi->first->Matches(*i))
			//if (i->GetDrawEnable())
			{
				//for (map <DRAWABLE_FILTER *, vector <SCENEDRAW> >::iterator mi = drawlist_output_map.begin(); mi != drawlist_output_map.end(); mi++)
				{
					//if (mi->first->Matches(*i))
					if (i->GetDrawEnable())
					{
						//mi->second.push_back(SCENEDRAW());
						//mi->second.back().SetCollapsed(*i,this_transform);
						mi->second.push_back(SCENEDRAW(*i,this_transform));
					}
				}
			}
		}
	}
	
	for (list <SCENENODE>::const_iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->GetCollapsedDrawList(drawlist_output_map, this_transform);
	}
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
	//if (draw_order != other.draw_order)
	//	return (draw_order < other.draw_order);
	
	/*if (diffuse_map.PtrValid() && other.diffuse_map.PtrValid())
		return (diffuse_map.GetPtr()->GetTexture().GetTextureInfo().GetName() < other.diffuse_map.GetPtr()->GetTexture().GetTextureInfo().GetName());*/
	
	//return false;
	
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
