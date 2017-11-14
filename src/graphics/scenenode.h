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

	Handle AddNode();

	SceneNode & GetNode(Handle handle);
	const SceneNode & GetNode(Handle handle) const;

	List & GetNodeList();
	const List & GetNodeList() const;

	DrawableList & GetDrawList();
	const DrawableList & GetDrawList() const;

	Transform & GetTransform();
	const Transform & GetTransform() const;

	void Clear();
	void Delete(Handle handle);

	Vec3 TransformIntoWorldSpace(const Vec3 & localspace) const;
	Vec3 TransformIntoLocalSpace(const Vec3 & worldspace) const;

	void SetChildVisibility(bool v);
	void SetChildAlpha(float a);

	template <class Stream>
	void DebugPrint(Stream & out, int curdepth = 0) const;

	template <template <typename U> class T>
	void Traverse(DrawableContainer <T> & drawlist_output, const Mat4 & prev_transform);

	/// traverse all drawable containers applying the specified functor.
	/// the functor should take a drawable container reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableContainerFunctor(T functor);

	/// traverse all drawables applying the specified functor.
	/// the functor should take any drawable typed reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableFunctor(T functor);

private:
	List childlist;
	DrawableList drawlist;
	Transform transform;
	Mat4 cached_transform;
};


inline SceneNode::Handle SceneNode::AddNode()
{
	return childlist.insert(SceneNode());
}

inline SceneNode & SceneNode::GetNode(SceneNode::Handle handle)
{
	return childlist.get(handle);
}

inline const SceneNode & SceneNode::GetNode(SceneNode::Handle handle) const
{
	return childlist.get(handle);
}

inline SceneNode::List & SceneNode::GetNodeList()
{
	return childlist;
}

inline const SceneNode::List & SceneNode::GetNodeList() const
{
	return childlist;
}

inline SceneNode::DrawableList & SceneNode::GetDrawList()
{
	return drawlist;
}

inline const SceneNode::DrawableList & SceneNode::GetDrawList() const
{
	return drawlist;
}

inline Transform & SceneNode::GetTransform()
{
	return transform;
}

inline const Transform & SceneNode::GetTransform() const
{
	return transform;
}

inline void SceneNode::Clear()
{
	drawlist.clear();
	childlist.clear();
}

inline void SceneNode::Delete(SceneNode::Handle handle)
{
	childlist.erase(handle);
}

inline Vec3 SceneNode::TransformIntoWorldSpace(const Vec3 & localspace) const
{
	Vec3 out(localspace);
	cached_transform.TransformVectorOut(out[0], out[1], out[2]);
	return out;
}

inline Vec3 SceneNode::TransformIntoLocalSpace(const Vec3 & worldspace) const
{
	Vec3 out(worldspace);
	cached_transform.TransformVectorIn(out[0], out[1], out[2]);
	return out;
}

inline void SceneNode::SetChildVisibility(bool v)
{
	drawlist.SetVisibility(v);

	for (auto & child : childlist)
	{
		child.SetChildVisibility(v);
	}
}

inline void SceneNode::SetChildAlpha(float a)
{
	drawlist.SetAlpha(a);

	for (auto & child : childlist)
	{
		child.SetChildAlpha(a);
	}
}

template <class Stream>
inline void SceneNode::DebugPrint(Stream & out, int curdepth) const
{
	for (int i = 0; i < curdepth; i++)
	{
		out << "-";
	}
	out << "Children: " << childlist.size() << ", Drawables: " << drawlist.size() << "\n";

	for (const auto & child : childlist)
	{
		child.DebugPrint(out, curdepth+1);
	}
}

template <template <typename U> class T>
inline void SceneNode::Traverse(DrawableContainer <T> & drawlist_output, const Mat4 & prev_transform)
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

	for (auto & child : childlist)
	{
		child.Traverse(drawlist_output, this_transform);
	}

	cached_transform = this_transform;
}

template <typename T>
inline void SceneNode::ApplyDrawableContainerFunctor(T functor)
{
	functor(drawlist);
	for (auto & child : childlist)
	{
		child.ApplyDrawableContainerFunctor(functor);
	}
}

template <typename T>
inline void SceneNode::ApplyDrawableFunctor(T functor)
{
	drawlist.ForEachDrawable(functor);
	for (auto & child : childlist)
	{
		child.ApplyDrawableFunctor(functor);
	}
}

#endif // _SCENENODE_H
