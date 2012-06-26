#ifndef _SPRITE2D_H
#define _SPRITE2D_H

#include "scenenode.h"
#include "vertexarray.h"

#include <string>
#include <cassert>
#include <iostream>

class ContentManager;

///a higher level class that takes care of using the TEXTURE, DRAWABLE, and VERTEXARRAY objects to create a 2D sprite
class SPRITE2D
{
private:
	VERTEXARRAY varray;
	keyed_container <DRAWABLE>::handle draw;
	keyed_container <SCENENODE>::handle node;
	float r, g, b, a;

	DRAWABLE & GetDrawableFromParent(SCENENODE & parent)
	{
		SCENENODE & noderef = GetNode(parent);
		return GetDrawableFromNode(noderef);
	}
	const DRAWABLE & GetDrawableFromParent(const SCENENODE & parent) const
	{
		const SCENENODE & noderef = GetNode(parent);
		return GetDrawableFromNode(noderef);
	}

	DRAWABLE & GetDrawableFromNode(SCENENODE & noderef)
	{
		return noderef.GetDrawlist().twodim.get(draw);
	}
	const DRAWABLE & GetDrawableFromNode(const SCENENODE & noderef) const
	{
		return noderef.GetDrawlist().twodim.get(draw);
	}

public:
	SPRITE2D() :
		r(1), g(1), b(1), a(1)
	{
		// ctor
	}

	DRAWABLE & GetDrawable(SCENENODE & parent) {return GetDrawableFromParent(parent);}

	SCENENODE & GetNode(SCENENODE & parent) {return parent.GetNode(node);}

	const SCENENODE & GetNode(const SCENENODE & parent) const {return parent.GetNode(node);}

	void Unload(SCENENODE & parent);

	bool Load(
		SCENENODE & parent,
		const std::string & texturepath,
		const std::string & texturename,
		ContentManager & content,
        float draworder);

	bool Load(
		SCENENODE & parent,
		std::tr1::shared_ptr<TEXTURE> texture2d,
		float draworder);

	///get the transformation data associated with this sprite's scenenode.
	///this can be used to get the current translation and rotation or set new ones.
	TRANSFORM & GetTransform(SCENENODE & parent)
	{
		SCENENODE & noderef = GetNode(parent);
		return noderef.GetTransform();
	}
	const TRANSFORM & GetTransform(const SCENENODE & parent) const
	{
		const SCENENODE & noderef = GetNode(parent);
		return noderef.GetTransform();
	}

	void SetAlpha(SCENENODE & parent, float na)
	{
		a=na;
		GetDrawableFromParent(parent).SetColor(r,g,b,a);
	}

	void SetVisible(SCENENODE & parent, bool newvis)
	{
		GetDrawableFromParent(parent).SetDrawEnable(newvis);
	}

	bool GetVisible(const SCENENODE & parent) const
	{
		return GetDrawableFromParent(parent).GetDrawEnable();
	}

	void SetColor(SCENENODE & parent, float nr, float ng, float nb)
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
