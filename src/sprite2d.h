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

#ifndef _SPRITE2D_H
#define _SPRITE2D_H

#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"

#include <memory>
#include <string>

class ContentManager;
class Texture;

/// A higher level class using Texture, Drawable, and VertexArray objects to create a 2D sprite
class Sprite2D
{
public:
	Sprite2D();

	bool Load(
		SceneNode & parent,
		const std::string & texturepath,
		const std::string & texturename,
		ContentManager & content,
		float draworder);

	bool Load(
		SceneNode & parent,
		std::shared_ptr<Texture> & texture2d,
		float draworder);

	void Unload(SceneNode & parent);

	bool Loaded() const;

	Drawable & GetDrawable(SceneNode & parent);

	const SceneNode & GetNode(const SceneNode & parent) const;

	SceneNode & GetNode(SceneNode & parent);

	/// get the transformation data associated with this sprite's scenenode.
	const Transform & GetTransform(const SceneNode & parent) const;

	Transform & GetTransform(SceneNode & parent);

	void SetVisible(SceneNode & parent, bool newvis);

	bool GetVisible(const SceneNode & parent) const;

	void SetColor(SceneNode & parent, float nr, float ng, float nb);

	void SetAlpha(SceneNode & parent, float na);

	float GetR() const;

	float GetG() const;

	float GetB() const;

	/// x and y represent the upper left corner.
	/// w and h are the width and height.
	void SetToBillboard(float x, float y, float w, float h);

	void SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip = false);

	void SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight);

	void SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z);

private:
	SceneNode::Handle node;
	SceneNode::DrawableHandle draw;
	std::shared_ptr<Texture> texture;
	VertexArray varray;
	float r, g, b, a;

	Drawable & GetDrawableFromParent(SceneNode & parent);

	const Drawable & GetDrawableFromParent(const SceneNode & parent) const;

	Drawable & GetDrawableFromNode(SceneNode & noderef);

	const Drawable & GetDrawableFromNode(const SceneNode & noderef) const;
};

// implementation

inline Sprite2D::Sprite2D() :
	r(1),
	g(1),
	b(1),
	a(1)
{
	// ctor
}

inline bool Sprite2D::Loaded() const
{
	return draw.valid();
}

inline Drawable & Sprite2D::GetDrawable(SceneNode & parent)
{
	return GetDrawableFromParent(parent);
}

inline const SceneNode & Sprite2D::GetNode(const SceneNode & parent) const
{
	return parent.GetNode(node);
}

inline SceneNode & Sprite2D::GetNode(SceneNode & parent)
{
	return parent.GetNode(node);
}

inline const Transform & Sprite2D::GetTransform(const SceneNode & parent) const
{
	const SceneNode & noderef = GetNode(parent);
	return noderef.GetTransform();
}

inline Transform & Sprite2D::GetTransform(SceneNode & parent)
{
	SceneNode & noderef = GetNode(parent);
	return noderef.GetTransform();
}

inline void Sprite2D::SetVisible(SceneNode & parent, bool newvis)
{
	GetDrawableFromParent(parent).SetDrawEnable(newvis);
}

inline bool Sprite2D::GetVisible(const SceneNode & parent) const
{
	return GetDrawableFromParent(parent).GetDrawEnable();
}

inline void Sprite2D::SetColor(SceneNode & parent, float nr, float ng, float nb)
{
	r = nr;
	g = ng;
	b = nb;
	GetDrawableFromParent(parent).SetColor(r, g, b, a);
}

inline void Sprite2D::SetAlpha(SceneNode & parent, float na)
{
	a = na;
	GetDrawableFromParent(parent).SetColor(r, g, b, a);
}

inline float Sprite2D::GetR() const
{
	return r;
}

inline float Sprite2D::GetG() const
{
	return g;
}

inline float Sprite2D::GetB() const
{
	return b;
}

inline void Sprite2D::SetToBillboard(float x, float y, float w, float h)
{
	varray.SetToBillboard(x, y, x + w, y + h);
}

inline void Sprite2D::SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip)
{
	varray.SetTo2DButton(x,y,w,h,sidewidth,flip);
}

inline void Sprite2D::SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight)
{
	varray.SetTo2DBox(x,y,w,h,marginwidth,marginheight);
}

inline void Sprite2D::SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z)
{
	varray.SetTo2DQuad(x1,y1,x2,y2,u1,v1,u2,v2,z);
}

inline Drawable & Sprite2D::GetDrawableFromParent(SceneNode & parent)
{
	SceneNode & noderef = GetNode(parent);
	return GetDrawableFromNode(noderef);
}

inline const Drawable & Sprite2D::GetDrawableFromParent(const SceneNode & parent) const
{
	const SceneNode & noderef = GetNode(parent);
	return GetDrawableFromNode(noderef);
}

inline Drawable & Sprite2D::GetDrawableFromNode(SceneNode & noderef)
{
	return noderef.GetDrawList().twodim.get(draw);
}

inline const Drawable & Sprite2D::GetDrawableFromNode(const SceneNode & noderef) const
{
	return noderef.GetDrawList().twodim.get(draw);
}

#endif
