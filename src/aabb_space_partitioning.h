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

#ifndef _AABB_SPACE_PARTITIONING_H
#define _AABB_SPACE_PARTITIONING_H

#include "aabb.h"
#include <list>
#include <iostream>

template <typename DATATYPE, unsigned int ideal_objects_per_node = 1>
class AABB_SPACE_PARTITIONING_NODE
{
public:
	void DebugPrint(int level, int & objectcount, bool verbose, std::ostream & output) const;

	unsigned int size(unsigned int objectcount = 0) const;

	void Optimize();

	void Add(DATATYPE & object, const AABB <float> & newaabb);

	/// A slow delete that only requires the object.
	void Delete(DATATYPE & object);

	/// A faster delete that uses the supplied AABB to find the object.
	void Delete(DATATYPE & object, const AABB <float> & objaabb);

	/// Run a query for objects that collide with the given shape.
	template <typename T, typename U>
	void Query(const T & shape, U &outputlist, bool testChildren=true) const;

	bool Empty() const;

	void Clear();

	/// Traverse the entire tree putting pointers to all DATATYPE objects into the given outputlist.
	void GetContainedObjects(std::list <DATATYPE *> & outputlist);

private:
	typedef std::vector <std::pair <DATATYPE, AABB <float> > > objectlist_type;
	typedef std::vector <AABB_SPACE_PARTITIONING_NODE> childrenlist_type;
	objectlist_type objects;
	childrenlist_type children;
	AABB <float> bbox;

	const AABB <float> & GetBBOX() const;

	/// Recursively send all objects and all childrens' objects to the target node, clearing out everything else.
	void CollapseTo(AABB_SPACE_PARTITIONING_NODE & collapse_target);

	void RemoveDuplicateObjects();

	/// Intelligently add new child nodes and parse objects to them, recursively.
	void DistributeObjectsToChildren(const int level);
};

#endif
