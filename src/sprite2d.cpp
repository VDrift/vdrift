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

#include "sprite2d.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"

#include <cassert>

bool Sprite2D::Load(
	SceneNode & parent,
	const std::string & texturepath,
	const std::string & texturename,
	ContentManager & content,
	float draworder)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());

	TextureInfo texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	texinfo.npot = false;
	content.load(texture, texturepath, texturename, texinfo);

	node = parent.AddNode();
	SceneNode & noderef = parent.GetNode(node);
	draw = noderef.GetDrawList().twodim.insert(Drawable());
	Drawable & drawref = GetDrawableFromNode(noderef);

	drawref.SetTextures(texture->GetId());
	drawref.SetVertArray(&varray);
	drawref.SetDrawOrder(draworder);
	drawref.SetCull(false);
	drawref.SetColor(r,g,b,a);

	//std::cout << "Sprite draworder: " << draworder << std::endl;

	return true;
}

bool Sprite2D::Load(
	SceneNode & parent,
	std::shared_ptr<Texture> & texture2d,
	float draworder)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());
	assert(texture2d);

	texture = texture2d;

	node = parent.AddNode();
	SceneNode & noderef = parent.GetNode(node);
	draw = noderef.GetDrawList().twodim.insert(Drawable());
	Drawable & drawref = GetDrawableFromNode(noderef);

	drawref.SetTextures(texture->GetId());
	drawref.SetVertArray(&varray);
	drawref.SetDrawOrder(draworder);
	drawref.SetCull(false);
	drawref.SetColor(r,g,b,a);

	return true;
}

void Sprite2D::Unload(SceneNode & parent)
{
	if (node.valid())
	{
		SceneNode & noderef = GetNode(parent);
		noderef.GetDrawList().twodim.erase(draw);
		parent.Delete(node);
	}
	node.invalidate();
	draw.invalidate();

	varray.Clear();
}
