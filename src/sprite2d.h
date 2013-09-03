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

#include <string>
#include <cassert>

class ContentManager;

/// A higher level class using Texture, Drawable, and VertexArray objects to create a 2D sprite
class Sprite2D
{
private:
	VertexArray varray;
	keyed_container <Drawable>::handle draw;
	keyed_container <SceneNode>::handle node;
	float r, g, b, a;

	Drawable & GetDrawableFromParent(SceneNode & parent)
	{
		SceneNode & noderef = GetNode(parent);
		return GetDrawableFromNode(noderef);
	}
	const Drawable & GetDrawableFromParent(const SceneNode & parent) const
	{
		const SceneNode & noderef = GetNode(parent);
		return GetDrawableFromNode(noderef);
	}

	Drawable & GetDrawableFromNode(SceneNode & noderef)
	{
		return noderef.GetDrawlist().twodim.get(draw);
	}
	const Drawable & GetDrawableFromNode(const SceneNode & noderef) const
	{
		return noderef.GetDrawlist().twodim.get(draw);
	}

public:
	Sprite2D() :
		r(1), g(1), b(1), a(1)
	{
		// ctor
	}

	Drawable & GetDrawable(SceneNode & parent) {return GetDrawableFromParent(parent);}

	SceneNode & GetNode(SceneNode & parent) {return parent.GetNode(node);}

	const SceneNode & GetNode(const SceneNode & parent) const {return parent.GetNode(node);}

	void Unload(SceneNode & parent);

	bool Load(
		SceneNode & parent,
		const std::string & texturepath,
		const std::string & texturename,
		ContentManager & content,
        float draworder);

	bool Load(
		SceneNode & parent,
		std::tr1::shared_ptr<Texture> texture2d,
		float draworder);

	///get the transformation data associated with this sprite's scenenode.
	///this can be used to get the current translation and rotation or set new ones.
	Transform & GetTransform(SceneNode & parent)
	{
		SceneNode & noderef = GetNode(parent);
		return noderef.GetTransform();
	}
	const Transform & GetTransform(const SceneNode & parent) const
	{
		const SceneNode & noderef = GetNode(parent);
		return noderef.GetTransform();
	}

	void SetAlpha(SceneNode & parent, float na)
	{
		a=na;
		GetDrawableFromParent(parent).SetColor(r,g,b,a);
	}

	void SetVisible(SceneNode & parent, bool newvis)
	{
		GetDrawableFromParent(parent).SetDrawEnable(newvis);
	}

	bool GetVisible(const SceneNode & parent) const
	{
		return GetDrawableFromParent(parent).GetDrawEnable();
	}

	void SetColor(SceneNode & parent, float nr, float ng, float nb)
	{
		r = nr;
		g = ng;
		b = nb;
		GetDrawableFromParent(parent).SetColor(r,g,b,a);
	}

	///x and y represent the upper left corner.
	///w and h are the width and height.
	void SetToBillboard(float x, float y, float w, float h)
	{
		varray.SetToBillboard(x, y, x+w, y+h);
	}

	void SetTo2DButton(float x, float y, float w, float h, float sidewidth, bool flip=false)
	{
		varray.SetTo2DButton(x,y,w,h,sidewidth,flip);
	}

	void SetTo2DBox(float x, float y, float w, float h, float marginwidth, float marginheight)
	{
		varray.SetTo2DBox(x,y,w,h,marginwidth,marginheight);
	}

	void SetTo2DQuad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, float z)
	{
		varray.SetTo2DQuad(x1,y1,x2,y2,u1,v1,u2,v2,z);
	}

	bool Loaded()
	{
		return (draw.valid());
	}

	float GetR() const
	{
		return r;
	}

	float GetG() const
	{
		return g;
	}

	float GetB() const
	{
		return b;
	}
};

#endif
