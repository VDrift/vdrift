#include "widget_image.h"

WIDGET * WIDGET_IMAGE::clone() const
{
	return new WIDGET_IMAGE(*this);
}

void WIDGET_IMAGE::SetAlpha(SCENENODE & node, float newalpha)
{
	GetDrawable(node).SetColor(1, 1, 1, newalpha);
}

void WIDGET_IMAGE::SetVisible(SCENENODE & node, bool newvis)
{
	GetDrawable(node).SetDrawEnable(newvis);
}

void WIDGET_IMAGE::SetupDrawable(
	SCENENODE & scene,
	const std::tr1::shared_ptr<TEXTURE> teximage,
	float x, float y, float w, float h,
	int order,
	bool button_mode,
	float screenhwratio)
{
	MATHVECTOR <float, 2> dim;
	dim.Set(w,h);
	MATHVECTOR <float, 2> center;
	center.Set(x,y);
	corner1 = center - dim*0.5;
	corner2 = center + dim*0.5;
	
	draw = scene.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(scene);
	drawref.SetDiffuseMap(teximage);
	drawref.SetVertArray(&varray);
	drawref.SetCull(false, false);
	drawref.SetColor(1,1,1,1);
	drawref.SetDrawOrder(order+100);
	
	if (button_mode)
		varray.SetTo2DButton(x, y, w, h, h/(screenhwratio*3.0));
		//varray.SetTo2DButton(x, y, w, h, h*0.25);
	else
		varray.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
}