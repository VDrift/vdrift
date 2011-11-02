#include "hudbar.h"
#include "scenenode.h"

void HUDBAR::Set(
	SCENENODE & parent,
	std::tr1::shared_ptr<TEXTURE> bartex,
	float x, float y, float w, float h,
	float opacity,
	bool flip)
{
	draw = parent.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);

	drawref.SetDiffuseMap(bartex);
	drawref.SetVertArray(&verts);
	drawref.SetCull(false, false);
	drawref.SetColor(1,1,1,opacity);
	drawref.SetDrawOrder(1);

	verts.SetTo2DButton(x, y, w, h, h*0.75, flip);
}

void HUDBAR::SetVisible(SCENENODE & parent, bool newvis)
{
	DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);
	drawref.SetDrawEnable(newvis);
}
