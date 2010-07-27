#include "sprite2d.h"

#include "contentmanager.h"
#include "textureloader.h"

#include <cassert>

bool SPRITE2D::Load(
	SCENENODE & parent,
	const std::string & texturefile,
	const std::string & texturesize,
	ContentManager & content,
	float draworder,
	std::ostream & error_output)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());

	TextureLoader texload;
	texload.name = texturefile;
	texload.mipmap = false;
	texload.repeatu = false;
	texload.repeatv = false;
	texload.npot = false;
	texload.size = texturesize;
	texture = content.get<TEXTURE>(texload);
	if (!texture.get()) return false;
	if (!texture.get()) return false;

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
	TexturePtr texture2d,
	float draworder,
	std::ostream & error_output)
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
