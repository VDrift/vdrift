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
#include "textureinfo.h"

void SPRITE2D::Unload(SCENENODE & parent)
{
	if (node.valid())
	{
		SCENENODE & noderef = GetNode(parent);
		noderef.GetDrawlist().twodim.erase(draw);
		parent.Delete(node);
	}
	node.invalidate();
	draw.invalidate();

	varray.Clear();
}

bool SPRITE2D::Load(
	SCENENODE & parent,
	const std::string & texturepath,
	const std::string & texturename,
	ContentManager & content,
	float draworder)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());

	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	texinfo.npot = false;
	std::tr1::shared_ptr<TEXTURE> texture;
	content.load(texture, texturepath, texturename, texinfo);

	node = parent.AddNode();
	SCENENODE & noderef = parent.GetNode(node);
	draw = noderef.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawableFromNode(noderef);

	drawref.SetDiffuseMap(texture);
	drawref.SetVertArray(&varray);
	drawref.SetDrawOrder(draworder);
	drawref.SetCull(false, false);
	drawref.SetColor(r,g,b,a);

	//std::cout << "Sprite draworder: " << draworder << std::endl;

	return true;
}

bool SPRITE2D::Load(
	SCENENODE & parent,
	std::tr1::shared_ptr<TEXTURE> texture2d,
	float draworder)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());

	node = parent.AddNode();
	SCENENODE & noderef = parent.GetNode(node);
	draw = noderef.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawableFromNode(noderef);

	drawref.SetDiffuseMap(texture2d);
	drawref.SetVertArray(&varray);
	drawref.SetDrawOrder(draworder);
	drawref.SetCull(false, false);
	drawref.SetColor(r,g,b,a);

	return true;
}
