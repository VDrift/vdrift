#ifndef _SCENENODE_H
#define _SCENENODE_H

#include "drawable_container.h"
#include "keyed_container.h"
#include "transform.h"

class SCENENODE
{
public:
	typedef MATRIX4<float> MAT4;
	typedef MATHVECTOR<float,3> VEC3;
	
	SCENENODE() : emptydrawlist(true),not_empty_count(0) {}
	
	keyed_container <SCENENODE>::handle AddNode() {return childlist.insert(SCENENODE());}
	SCENENODE & GetNode(keyed_container <SCENENODE>::handle handle) {return childlist.get(handle);}
	const SCENENODE & GetNode(keyed_container <SCENENODE>::handle handle) const {return childlist.get(handle);}
	
	DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() {emptydrawlist=false;return drawlist;}
	const DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() const {return drawlist;}
	
	TRANSFORM & GetTransform() {return transform;}
	void SetTransform(const TRANSFORM & newtransform) {transform=newtransform;}
	const TRANSFORM & GetTransform() const {return transform;}
	unsigned int Nodes() const {return childlist.size();}
	unsigned int Drawables() const {return drawlist.size();}
	void Clear() {drawlist.clear();childlist.clear();emptydrawlist=true;}
	void Delete(keyed_container <SCENENODE>::handle handle) {childlist.erase(handle);}
	VEC3 TransformIntoWorldSpace() const {VEC3 zero;return TransformIntoWorldSpace(zero);}
	VEC3 TransformIntoWorldSpace(const VEC3 & localspace) const;
	VEC3 TransformIntoLocalSpace(const VEC3 & worldspace) const;
	void SetChildVisibility(bool newvis);
	void SetChildAlpha(float a);
	void DebugPrint(std::ostream & out, int curdepth = 0) const;
	
	template <template <typename U> class T>
	void Traverse(DRAWABLE_CONTAINER <T> & drawlist_output, const MAT4 & prev_transform)
	{
		// emptydrawlist is a cached value that if true says _for sure_ our drawlist is empty.
		// if emptydrawlist is false, we don't know for sure whether or not the list is empty,
		// so we must get the actual value. as an additional optimization, stop trying to
		// early-out if we've failed to early-out at least a certain number of times.
		if (not_empty_count < 5)
		{
			if (childlist.empty())
			{
				if (emptydrawlist)
				{
					return;
				}
				else if (drawlist.empty())
				{
					emptydrawlist = true;
					return;
				}
				else
					not_empty_count++;
			}
			else
				not_empty_count++;
		}
		
		MAT4 this_transform(prev_transform);
		
		bool identitytransform = transform.IsIdentityTransform();
		if (!identitytransform)
		{
			transform.GetRotation().GetMatrix4(this_transform);
			this_transform.Translate(transform.GetTranslation()[0], transform.GetTranslation()[1], transform.GetTranslation()[2]);
			this_transform = this_transform.Multiply(prev_transform);
		}
		
		if (this_transform != cached_transform)
			drawlist.AppendTo<T,true>(drawlist_output, this_transform);
		else
			drawlist.AppendTo<T,false>(drawlist_output, this_transform);
		
		for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->Traverse(drawlist_output, this_transform);
		}
		
		cached_transform = this_transform;
	}
	
	/// traverse all drawable containers applying the specified functor.
	/// the functor should take a drawable container reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableContainerFunctor(T functor)
	{
		functor(drawlist);
		for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->ApplyDrawableContainerFunctor(functor);
		}
	}
	
	/// traverse all drawables applying the specified functor.
	/// the functor should take any drawable typed reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableFunctor(T functor)
	{
		drawlist.ForEachDrawable(functor);
		for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->ApplyDrawableFunctor(functor);
		}
	}
	
private:
	keyed_container <SCENENODE> childlist;
	DRAWABLE_CONTAINER <keyed_container> drawlist;
	bool emptydrawlist;
	int not_empty_count;
	TRANSFORM transform;
	MAT4 cached_transform;
};

#endif // _SCENENODE_H
