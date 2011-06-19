#ifndef _STATICDRAWABLES_H
#define _STATICDRAWABLES_H

#include "aabb_space_partitioning.h"
#include "scenenode.h"

#include <vector>

#define OBJECTS_PER_NODE 64

template <typename T>
class AABB_SPACE_PARTITIONING_NODE_ADAPTER
{
friend class STATICDRAWABLES;
public:
	AABB_SPACE_PARTITIONING_NODE_ADAPTER() : count(0) {}

	void push_back(T * drawable)
	{
		MATHVECTOR <float, 3> objpos(drawable->GetObjectCenter());
		drawable->GetTransform().TransformVectorOut(objpos[0],objpos[1],objpos[2]);
		float radius = drawable->GetRadius();
		AABB <float> box;
		box.SetFromSphere(objpos, radius);
		spacetree.Add(drawable, box);
	}
	unsigned int size() const {return count;}
	void clear() {spacetree.Clear();}
	void Optimize() {spacetree.Optimize();count=spacetree.size();}
	template <typename U>
	void Query(const U & object, std::vector <T*> & output) const {spacetree.Query(object, output);}

private:
	AABB_SPACE_PARTITIONING_NODE <T*,OBJECTS_PER_NODE> spacetree;
	unsigned int count; ///< cached from spacetree.size()
};

class STATICDRAWABLES
{
public:
	void Generate(SCENENODE & node, bool clearfirst = true)
	{
		if (clearfirst)
			drawables.clear();

		MATRIX4 <float> identity;
		node.Traverse(drawables, identity);

		drawables.ForEach(OptimizeFunctor());
	}

	DRAWABLE_CONTAINER <AABB_SPACE_PARTITIONING_NODE_ADAPTER> & GetDrawlist() {return drawables;}

private:
	DRAWABLE_CONTAINER <AABB_SPACE_PARTITIONING_NODE_ADAPTER> drawables;

	struct OptimizeFunctor
	{
		template <typename T>
		void operator()(AABB_SPACE_PARTITIONING_NODE_ADAPTER <T> & container)
		{
			container.Optimize();
		}
	};
};

#undef OBJECTS_PER_NODE

#endif
