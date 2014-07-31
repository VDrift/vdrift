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

#ifndef _AABB_TREE_ADAPTER_H
#define _AABB_TREE_ADAPTER_H

#include "aabbtree.h"
#include <vector>

#define OBJECTS_PER_NODE 64

template <typename T>
class AabbTreeNodeAdapter
{
public:
	AabbTreeNodeAdapter() : count(0) {}

	void push_back(T * drawable)
	{
		Vec3 objpos(drawable->GetObjectCenter());
		drawable->GetTransform().TransformVectorOut(objpos[0],objpos[1],objpos[2]);
		float radius = drawable->GetRadius();
		Aabb <float> box;
		box.SetFromSphere(objpos, radius);
		spacetree.Add(drawable, box);
	}

	unsigned int size() const
	{
		return count;
	}

	void clear()
	{
		spacetree.Clear();
	}

	void Optimize()
	{
		spacetree.Optimize();
		count = spacetree.size();
	}

	template <typename U>
	void Query(const U & object, std::vector <T*> & output) const
	{
		spacetree.Query(object, output);
	}

private:
	AabbTreeNode <T*,OBJECTS_PER_NODE> spacetree;
	unsigned int count; ///< cached from spacetree.size()
};

/// adapter helper functor
struct OptimizeFunctor
{
	template <typename T>
	void operator()(AabbTreeNodeAdapter <T> & container)
	{
		container.Optimize();
	}
};

#undef OBJECTS_PER_NODE

#endif
