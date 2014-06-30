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

#ifndef _SCENENODE_H
#define _SCENENODE_H

#include "drawable_container.h"
#include "keyed_container.h"
#include "transform.h"

class SceneNode
{
public:
	typedef DrawableContainer <keyed_container> DrawableList;
	typedef keyed_container<Drawable>::handle DrawableHandle;
	typedef keyed_container<SceneNode> List;
	typedef List::handle Handle;

	Handle AddNode() {return childlist.insert(SceneNode());}

	SceneNode & GetNode(Handle handle) {return childlist.get(handle);}
	const SceneNode & GetNode(Handle handle) const {return childlist.get(handle);}

	List & GetNodeList() {return childlist;}
	const List & GetNodeList() const {return childlist;}

	DrawableList & GetDrawList() {return drawlist;}
	const DrawableList & GetDrawList() const {return drawlist;}

	Transform & GetTransform() {return transform;}
	const Transform & GetTransform() const {return transform;}

	void Clear() {drawlist.clear();childlist.clear();}

	void Delete(Handle handle) {childlist.erase(handle);}

	Vec3 TransformIntoWorldSpace(const Vec3 & localspace) const;
	Vec3 TransformIntoLocalSpace(const Vec3 & worldspace) const;

	void SetChildVisibility(bool newvis);

	void SetChildAlpha(float a);

	void DebugPrint(std::ostream & out, int curdepth = 0) const;

	template <template <typename U> class T>
	void Traverse(DrawableContainer <T> & drawlist_output, const Mat4 & prev_transform)
	{
		Mat4 this_transform(prev_transform);

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

		for (List::iterator i = childlist.begin(); i != childlist.end(); ++i)
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
		for (List::iterator i = childlist.begin(); i != childlist.end(); ++i)
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
		for (List::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->ApplyDrawableFunctor(functor);
		}
	}

private:
	List childlist;
	DrawableList drawlist;
	Transform transform;
	Mat4 cached_transform;
};

#endif // _SCENENODE_H
