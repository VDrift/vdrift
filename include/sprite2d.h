#ifndef _SPRITE2D_H
#define _SPRITE2D_H

#include <string>
#include <cassert>
#include <iostream>

#include "scenegraph.h"
#include "vertexarray.h"
#include "texture.h"

///a higher level class that takes care of using the TEXTURE, DRAWABLE, and VERTEXARRAY objects to create a 2D sprite
class SPRITE2D
{
private:
	VERTEXARRAY varray;
	TEXTURE_GL texture;
	DRAWABLE * draw;
	SCENENODE * node;
	float r,g,b,a;

	void SendColor()
	{
		assert(draw);
		draw->SetColor(r,g,b,a);
	}

public:
	SPRITE2D() : draw(NULL),node(NULL),r(1),g(1),b(1),a(1) {}

	DRAWABLE * GetDrawable() {return draw;}
	SCENENODE * GetNode() {return node;}

	float GetW() const
	{
	    return texture.GetW();
	}

	float GetH() const
	{
	    return texture.GetH();
	}

	float GetOriginalW() const
	{
	    return texture.GetOriginalW();
	}

	float GetOriginalH() const
	{
	    return texture.GetOriginalH();
	}

	void Unload(SCENENODE * parent)
	{
		assert(parent);
		if (node && draw)
			node->Delete(draw);
		if (node)
			parent->Delete(node);
		node = NULL;
		draw = NULL;

		if (texture.Loaded())
			texture.Unload();

		varray.Clear();
	}

	bool Load(SCENENODE * parent, const std::string & texturefile, const std::string & texturesize,
		  float draworder, std::ostream & error_output)
	{
		assert(parent);

		Unload(parent);

		assert(!draw);
		assert(!node);

		TEXTUREINFO texinfo(texturefile);
		texinfo.SetMipMap(false);
		texinfo.SetRepeat(false, false);
		texinfo.SetAllowNonPowerOfTwo(false);
		if (!texture.Load(texinfo, error_output, texturesize))
			return false;

		node = &parent->AddNode();
		draw = &node->AddDrawable();
		assert(node);
		assert(draw);

		draw->SetDiffuseMap(&texture);
		draw->SetVertArray(&varray);
		draw->SetDrawOrder(draworder);
		draw->SetLit(false);
		draw->Set2D(true);
		draw->SetCull(false, false);
		draw->SetPartialTransparency(true);
		SendColor();

		//std::cout << "Sprite draworder: " << draworder << std::endl;

		return true;
	}

	///get the transformation data associated with this sprite's scenenode.
	///this can be used to get the current translation and rotation or set new ones.
	SCENETRANSFORM & GetTransform()
	{
		assert(node);
		return node->GetTransform();
	}
	const SCENETRANSFORM & GetTransform() const
	{
		assert(node);
		return node->GetTransform();
	}

	///use the provided texture
	bool Load(SCENENODE * parent, TEXTURE_GL * texture2d, float draworder, std::ostream & error_output)
	{
		assert(parent);

		Unload(parent);

		assert(!draw);
		assert(!node);

		node = &parent->AddNode();
		draw = &node->AddDrawable();
		assert(node);
		assert(draw);

		draw->SetDiffuseMap(texture2d);
		draw->SetVertArray(&varray);
		draw->SetDrawOrder(draworder);
		draw->SetLit(false);
		draw->Set2D(true);
		draw->SetCull(false, false);
		draw->SetPartialTransparency(true);
		SendColor();

		//std::cout << "Sprite draworder: " << draworder << std::endl;

		return true;
	}

	void SetAlpha(float na)
	{
		a = na;
		SendColor();
		//std::cout << "Sprite alpha: " << a << std::endl;
	}

	void SetVisible(bool newvis)
	{
		assert(draw);
		draw->SetDrawEnable(newvis);
	}
	
	bool GetVisible() const
	{
		assert(draw);
		return draw->GetDrawEnable();
	}

	void SetColor(float nr, float ng, float nb)
	{
		r = nr;
		g = ng;
		b = nb;
		SendColor();
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
		return (draw != NULL);
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
