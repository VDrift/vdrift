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

#ifndef _AABBTREE_H
#define _AABBTREE_H

#include "aabb.h"
#include "mathvector.h"

#include <vector>
#include <iostream> // std::cout

template <typename DataType, unsigned int ideal_objects_per_node = 1>
class AabbTreeNode
{
public:
	template <class Stream>
	void DebugPrint(int level, int & objectcount, bool verbose, Stream & output) const
	{
		if (verbose)
		{
			for (int i = 0; i < level; ++i) output << "-";

			output << "objects: " << objects.size() << ", child nodes: " << children.size() << ", aabb: ";
			bbox.DebugPrint(output);
		}

		objectcount += objects.size();

		for (const auto & child : children)
		{
			child.DebugPrint(level+1, objectcount, verbose, output);
		}

		if (level == 0)
		{
			if (verbose)
			{
				output << "================\n";
			}
			output << "TOTAL OBJECTS: " << objectcount << "\n";
		}
	}

	unsigned int size(unsigned int objectcount = 0) const
	{
		unsigned int childcount = 0;

		for (const auto & child : children)
		{
			childcount += child.size(objectcount);
		}

		objectcount += objects.size() + childcount;

		return objectcount;
	}

	void Optimize()
	{
		CollapseTo(*this);

		RemoveDuplicateObjects();

		DistributeObjectsToChildren(0);
	}

	void Add(DataType & object, const Aabb <float> & newaabb)
	{
		objects.push_back(std::make_pair(object, newaabb));
		if (objects.size() == 1) //don't combine if this is the first object, otherwise the AABB would be forced to include (0,0,0)
			bbox = newaabb;
		else
			bbox.CombineWith(newaabb);
	}

	///a slow delete that only requires the object
	void Delete(DataType & object)
	{
		//delete using swap and pop
		auto i = objects.rbegin();
		while (i != objects.rend())
		{
			auto k = i++;
			if (k->first == object)
			{
				if (k != objects.rbegin())
					*k = objects.back();
				objects.pop_back();
			}
		}

		//if we have children, pass it on
		for (auto & child : children)
		{
			child.Delete(object);
		}
	}

	///a faster delete that uses the supplied AABB to find the object
	void Delete(DataType & object, const Aabb <float> & objaabb)
	{
		//if we've got objects, test them
		auto i = objects.rbegin();
		while (i != objects.rend())
		{
			auto k = i++;
			if (k->first == object)
			{
				if (k != objects.rbegin())
					*k = objects.back();
				objects.pop_back();
			}
		}

		//if we have children, pass it on
		for (auto child : children)
		{
			if (child.GetBBOX().Intersect(objaabb))
			{
				child.Delete(object, objaabb);
			}
		}
	}

	///run a query for objects that collide with the given shape
	template <typename T, typename U>
	void Query(const T & shape, U &outputlist, bool testChildren=true) const
	{
		//if we've got objects, test them
		if (objects.size() > 1 && testChildren)
		{
			for (const auto & object : objects)
			{
				if (object.second.Intersect(shape) != Aabb<float>::OUT)
				{
					outputlist.push_back(object.first);
				}
			}
		}
		else
		{
			for (const auto & object : objects)
			{
				outputlist.push_back(object.first);
			}
		}

		//if we have children, test them
		for (const auto & child : children)
		{
			Aabb<float>::IntersectionEnum intersection = child.GetAabb().Intersect(shape);

			if (intersection != Aabb<float>::OUT)
			{
				//our child intersects with the segment, dispatch a query
				//only bother to test children if we were partially intersecting (if fully in, then we know all children are fully in too)
				child.Query(shape, outputlist, intersection == Aabb<float>::INTERSECT);
			}
		}
	}

	bool Empty() const {return (objects.empty() && children.empty());}

	void Clear() {objects.clear(); children.clear();}

	///traverse the entire tree putting pointers to all DataType objects into the given outputlist
	template <class List>
	void GetContainedObjects(List & outputlist)
	{
		//if we've got objects, add them
		for (auto & object : objects)
		{
			outputlist.push_back(&object.first);
		}

		//if we have children, add them
		for (auto & child : children)
		{
			child.GetContainedObjects(outputlist);
		}
	}

private:
	typedef std::vector <std::pair <DataType, Aabb <float> > > objectlist_type;
	typedef std::vector <AabbTreeNode> childrenlist_type;
	objectlist_type objects;
	childrenlist_type children;
	Aabb <float> bbox;

	const Aabb <float> & GetAabb() const {return bbox;}

	///recursively send all objects and all childrens' objects to the target node, clearing out everything else
	void CollapseTo(AabbTreeNode & collapse_target)
	{
		if (this != &collapse_target)
		{
			for (auto & object : objects)
			{
				collapse_target.Add(object.first, object.second);
			}
			objects.clear();
		}

		for (auto & child : children)
		{
			child.CollapseTo(collapse_target);
		}
		children.clear();
	}

	///requires the DataType class implements operator< and operator==
	void RemoveDuplicateObjects()
	{
		//TODO:  re-enable this code and make it compile
		//objects.sort();
		//objects.unique();
	}

	///intelligently add new child nodes and parse objects to them, recursively
	void DistributeObjectsToChildren(const int level)
	{
		const unsigned int ideal_children_per_node(2);
		const bool verbose(false);

		//enforce the rules:  i don't want to start with any children,
		// so tell all children to send me their objects, then delete them
		for (auto & child : children)
		{
			child.CollapseTo(*this);
		}
		children.clear();

		//only continue if we have more objects than we need
		if (objects.size() <= ideal_objects_per_node) return;

		children.resize(ideal_children_per_node);

		//determine the average center position of all objects
		Vec3 avgcenter;
		int numobj = objects.size();
		float incamount = 1.0 / numobj;
		for (const auto & object : objects)
		{
			avgcenter = avgcenter + object.second.GetCenter() * incamount;
		}

		//find axis of maximum change, so we know where to split
		Vec3 axismask(1,0,0);
		Vec3 extent = bbox.GetExtent();
		if (extent[0] > extent[1] && extent[0] > extent[2])
		{
			axismask.Set(1,0,0);
		}
		else if (extent[1] > extent[0] && extent[1] > extent[2])
		{
			axismask.Set(0,1,0);
		}
		else if (extent[2] > extent[1] && extent[2] > extent[0])
		{
			axismask.Set(0,0,1);
		}

		//cout << level << endl;

		//distribute objects to each child
		float avgcentercoord = avgcenter.dot(axismask);
		int distributor(0);
		for (auto & object : objects)
		{
			float objcentercoord = object.second.GetCenter().dot(axismask);

			if (objcentercoord > avgcentercoord)
			{
				children.front().Add(object.first, object.second);
			}
			else if (objcentercoord < avgcentercoord)
			{
				children.back().Add(object.first, object.second);
			}
			else
			{
				//cout << "unusual case #" << distributor << endl;
				//distribute children that sit right on our average center in an even way
				if (distributor % 2 == 0)
				{
					children.front().Add(object.first, object.second);
				}
				else
				{
					children.back().Add(object.first, object.second);
				}
				distributor++;
			}
		}

		//we've given away all of our objects; clear them out
		objects.clear();

		//count objects that belong to our children
		int child1obj = children.front().objects.size();
		int child2obj = children.back().objects.size();

		if (verbose) std::cout << "Objects: " << objects.size() << ", Child nodes: " << children.size() << " L obj: " << child1obj << " R obj: " << child2obj << std::endl;

		//if one child doesn't have any objects, then delete both children and take back their objects
		if (child1obj == 0 || child2obj == 0)
		{
			for (auto & child : children)
			{
				for (auto & object : child.objects)
				{
					Add(object.first, object.second);
				}
			}

			children.clear();
		}

		for (auto & child : children)
		{
			child.DistributeObjectsToChildren(level + 1);
		}
	}
};

#endif // _AABBTREE_H
