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

#include "aabb_space_partitioning.h"
#include "drawable.h"
#include "unittest.h"

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::DebugPrint(int level, int & objectcount, bool verbose, std::ostream & output) const
{
	if (verbose)
	{
		for (int i = 0; i < level; ++i)
			output << "-";

		output << "objects: " << objects.size() << ", child nodes: " << children.size() << ", aabb: ";
		bbox.DebugPrint(output);
	}

	objectcount += objects.size();

	for (typename childrenlist_type::const_iterator i = children.begin(); i != children.end(); ++i)
		i->DebugPrint(level+1, objectcount, verbose, output);

	if (level == 0)
	{
		if (verbose)
			output << "================" << std::endl;
		output << "TOTAL OBJECTS: " << objectcount << std::endl;
	}
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
unsigned int AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::size(unsigned int objectcount) const
{
	unsigned int childcount = 0;

	for (typename childrenlist_type::const_iterator i = children.begin(); i != children.end(); ++i)
		childcount += i->size(objectcount);

	objectcount += objects.size() + childcount;

	return objectcount;
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Optimize()
{
	CollapseTo(*this);

	RemoveDuplicateObjects();

	DistributeObjectsToChildren(0);
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Add(DATATYPE & object, const AABB <float> & newaabb)
{
	objects.push_back(std::pair <DATATYPE, AABB <float> > (object, newaabb));
	// Don't combine if this is the first object, otherwise the AABB would be forced to include (0,0,0).
	if (objects.size() == 1)
		bbox = newaabb;
	else
		bbox.CombineWith(newaabb);
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Delete(DATATYPE & object)
{
	typename std::list <typename objectlist_type::iterator> todel;

	// If we've got objects, test them.
	for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
		if (i->first == object)
			todel.push_back(i);

	// Do any deletions.
	for (typename std::list <typename objectlist_type::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		objects.erase(*i);

	// If we have children, pass it on.
	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		i->Delete(object);
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Delete(DATATYPE & object, const AABB <float> & objaabb)
{
	typename std::list <typename objectlist_type::iterator> todel;

	// If we've got objects, test them.
	for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
		if (i->first == object)
			todel.push_back(i);

	// Do any deletions.
	for (typename std::list <typename objectlist_type::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		objects.erase(*i);

	// If we have children, pass it on.
	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		if (i->GetBBOX().Intersect(objaabb))
			i->Delete(object, objaabb);
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
template <typename T, typename U>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Query(const T & shape, U &outputlist, bool testChildren) const
{
	// If we've got objects, test them.
	if (objects.size() > 1 && testChildren)
		for (typename objectlist_type::const_iterator i = objects.begin(); i != objects.end(); ++i)
			if (i->second.Intersect(shape) != AABB<float>::OUT)
				outputlist.push_back(i->first);
			else
				for (typename objectlist_type::const_iterator i = objects.begin(); i != objects.end(); ++i)
					outputlist.push_back(i->first);

	// If we have children, test them.
	for (typename childrenlist_type::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		AABB<float>::INTERSECTION intersection = i->GetBBOX().Intersect(shape);

		if (intersection != AABB<float>::OUT)
			// Our child intersects with the segment, dispatch a query.
			// Only bother to test children if we were partially intersecting (if fully in, then we know all children are fully in too).
			i->Query(shape, outputlist, intersection == AABB<float>::INTERSECT);
	}
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
bool AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Empty() const
{
	return (objects.empty() && children.empty());
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::Clear()
{
	objects.clear();
	children.clear();
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::GetContainedObjects(std::list <DATATYPE *> & outputlist)
{
	// If we've got objects, add them.
	for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
		outputlist.push_back(&i->first);

	// If we have children, add them.
	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		i->GetContainedObjects(outputlist);
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
const AABB <float> & AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::GetBBOX() const
{
	return bbox;
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::CollapseTo(AABB_SPACE_PARTITIONING_NODE & collapse_target)
{
	if (this != &collapse_target)
	{
		for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
			collapse_target.Add(i->first, i->second);
		objects.clear();
	}

	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		i->CollapseTo(collapse_target);
	children.clear();
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::RemoveDuplicateObjects()
{
	//TODO: Re-enable this code and make it compile. Requires the DATATYPE class implements operator< and operator==.
	//objects.sort();
	//objects.unique();
}

template <typename DATATYPE, unsigned int ideal_objects_per_node>
void AABB_SPACE_PARTITIONING_NODE<DATATYPE, ideal_objects_per_node>::DistributeObjectsToChildren(const int level)
{
	const unsigned int ideal_children_per_node(2);

	// Enforce the rules: I don't want to start with any children, so tell all children to send me their objects, then delete them.
	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		i->CollapseTo(*this);
	children.clear();

	// Only continue if we have more objects than we need.
	if (objects.size() <= ideal_objects_per_node)
		return;

	children.resize(ideal_children_per_node);

	// Determine the average center position of all objects.
	MATHVECTOR <float, 3> avgcenter;
	float incamount = 1.0 / objects.size();
	for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
		avgcenter = avgcenter + i->second.GetCenter() * incamount;

	// Find axis of maximum change, so we know where to split.
	MATHVECTOR <float, 3> axismask(1,0,0);
	MATHVECTOR <float, 3> bboxsize = bbox.GetSize();
	if (bboxsize[0] > bboxsize[1] && bboxsize[0] > bboxsize[2])
		axismask.Set(1,0,0);
	else if (bboxsize[1] > bboxsize[0] && bboxsize[1] > bboxsize[2])
		axismask.Set(0,1,0);
	else if (bboxsize[2] > bboxsize[1] && bboxsize[2] > bboxsize[0])
		axismask.Set(0,0,1);

	// Distribute objects to each child.
	float avgcentercoord = avgcenter.dot(axismask);
	int distributor(0);
	for (typename objectlist_type::iterator i = objects.begin(); i != objects.end(); ++i)
	{
		float objcentercoord = i->second.GetCenter().dot(axismask);

		if (objcentercoord > avgcentercoord)
			children.front().Add(i->first, i->second);
		else if (objcentercoord < avgcentercoord)
			children.back().Add(i->first, i->second);
		else if (distributor % 2 == 0)
		{
			// Distribute children that sit right on our average center in an even way.
			children.front().Add(i->first, i->second);
			distributor++;
		}
		else
		{
			children.back().Add(i->first, i->second);
			distributor++;
		}
	}

	// We've given away all of our objects; clear them out.
	objects.clear();

	// Count objects that belong to our children.
	int child1obj = children.front().objects.size();
	int child2obj = children.back().objects.size();

	// If one child doesn't have any objects, then delete both children and take back their objects.
	if (child1obj == 0 || child2obj == 0)
	{
		for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
			for (typename objectlist_type::iterator n = i->objects.begin(); n != i->objects.end(); n++)
				Add(n->first, n->second);

		children.clear();
	}

	for (typename childrenlist_type::iterator i = children.begin(); i != children.end(); ++i)
		i->DistributeObjectsToChildren(level + 1);
}

// This is used for oblying the compiler to generate the corresponding classes and functions for avoiding linker errors.
template class AABB_SPACE_PARTITIONING_NODE<int, 1>;
template class AABB_SPACE_PARTITIONING_NODE<DRAWABLE*, 64>;
template void AABB_SPACE_PARTITIONING_NODE<DRAWABLE*, 64>::Query<FRUSTUM, std::vector<DRAWABLE*, std::allocator<DRAWABLE*> > >(FRUSTUM const&, std::vector<DRAWABLE*, std::allocator<DRAWABLE*> >&, bool) const;
template void AABB_SPACE_PARTITIONING_NODE<DRAWABLE*, 64>::Query<AABB<float>::INTERSECT_ALWAYS, std::vector<DRAWABLE*, std::allocator<DRAWABLE*> > >(AABB<float>::INTERSECT_ALWAYS const&, std::vector<DRAWABLE*, std::allocator<DRAWABLE*> >&, bool) const;
template void AABB_SPACE_PARTITIONING_NODE<int, 1>::Query<AABB<float>::RAY, std::vector<int, std::allocator<int> > >(AABB<float>::RAY const&, std::vector<int, std::allocator<int> >&, bool) const;

QT_TEST(aabb_space_partitioning_test)
{
	AABB_SPACE_PARTITIONING_NODE <int> testnode;
	QT_CHECK_EQUAL(testnode.size(), 0);
}
