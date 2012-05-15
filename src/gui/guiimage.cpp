#include "gui/guiimage.h"

void GUIIMAGE::SetAlpha(SCENENODE & node, float newalpha)
{
	GetDrawable(node).SetColor(1, 1, 1, newalpha);
}

void GUIIMAGE::SetVisible(SCENENODE & node, bool newvis)
{
	GetDrawable(node).SetDrawEnable(newvis);
}

void GUIIMAGE::SetupDrawable(
	SCENENODE & scene,
	const std::tr1::shared_ptr<TEXTURE> teximage,
	float x, float y, float w, float h, int z,
	bool button_mode,
	float screenhwratio)
{
	float r(1), g(1), b(1), a(1);
	MATHVECTOR <float, 2> dim(w, h);
	MATHVECTOR <float, 2> center(x, y);
	corner1 = center - dim * 0.5;
	corner2 = center + dim * 0.5;

	draw = scene.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetDiffuseMap(teximage);
	drawref.SetVertArray(&varray);
	drawref.SetCull(false, false);
	drawref.SetColor(r, g, b, a);
	drawref.SetDrawOrder(z);

	if (button_mode)
	{
		float sidewidth = h / (screenhwratio * 3.0);
		varray.SetTo2DButton(x, y, w, h, sidewidth);
		corner1[0] -= sidewidth;
		corner2[0] += sidewidth;
	}
	else
	{
		varray.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
	}
}
