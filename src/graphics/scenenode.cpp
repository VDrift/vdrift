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

#include "scenenode.h"

Vec3 SceneNode::TransformIntoWorldSpace(const Vec3 & localspace) const
{
	Vec3 out(localspace);
	cached_transform.TransformVectorOut(out[0], out[1], out[2]);
	return out;
}

Vec3 SceneNode::TransformIntoLocalSpace(const Vec3 & worldspace) const
{
	Vec3 out(worldspace);
	cached_transform.TransformVectorIn(out[0], out[1], out[2]);
	return out;
}

void SceneNode::SetChildVisibility(bool newvis)
{
	drawlist.SetVisibility(newvis);

	for (List::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildVisibility(newvis);
	}
}

void SceneNode::SetChildAlpha(float a)
{
	drawlist.SetAlpha(a);

	for (List::iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->SetChildAlpha(a);
	}
}

void SceneNode::DebugPrint(std::ostream & out, int curdepth) const
{
	for (int i = 0; i < curdepth; i++)
		out << "-";
	out << "Children: " << childlist.size() << ", Drawables: " << drawlist.size() << std::endl;

	for (List::const_iterator i = childlist.begin(); i != childlist.end(); ++i)
	{
		i->DebugPrint(out, curdepth+1);
	}
}
