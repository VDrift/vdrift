#include "sprite2d.h"

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
	const std::string & texturefile,
	const std::string & texturesize,
	MANAGER<TEXTURE, TEXTUREINFO> & textures,
	float draworder,
	std::ostream & error_output)
{
	Unload(parent);

	assert(!draw.valid());
	assert(!node.valid());

	TEXTUREINFO texinfo;
	texinfo.SetName(texturefile);
	texinfo.SetMipMap(false);
	texinfo.SetRepeat(false, false);
	texinfo.SetAllowNonPowerOfTwo(false);
	texinfo.SetSize(texturesize);
	std::tr1::shared_ptr<TEXTURE> texture(textures.Get(texinfo)); 
	if (!texture->Loaded()) return false;

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