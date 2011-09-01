#include "loadingscreen.h"
#include "contentmanager.h"
#include "textureinfo.h"

void LOADINGSCREEN::Update(float percentage, const std::string & optional_text, float posx, float posy)
{
	if (percentage < 0)
		percentage = 0;
	if (percentage > 1.0)
		percentage = 1.0;

	if (optional_text.empty())
	{
		boxverts.SetTo2DButton(0,0,w,h,w*0.5/3.,false);

		float barheight = h*0.5*hscale;
		float y1 = -barheight;
		float y2 = barheight;
		barverts.SetToBillboard(-w*0.5,y1,-w*0.5+w*percentage, y2);
		barbackverts.SetToBillboard(-w*0.5,y1,w*0.5, y2);
	}
	else
	{
		boxverts.SetTo2DButton(0,0,w,h*1.5,w*0.5/3.,false);

		float barheight = h*0.5*hscale;
		float y1 = 0;
		float y2 = barheight*2;
		barverts.SetToBillboard(-w*0.5,y1,-w*0.5+w*percentage, y2);
		barbackverts.SetToBillboard(-w*0.5,y1,w*0.5, y2);
	}

	text.Revise(optional_text);

	root.GetTransform().SetTranslation(MATHVECTOR<float,3>(posx,posy,0));
}

///initialize the loading screen given the root node for the loading screen
bool LOADINGSCREEN::Init(
	const std::string & texturepath,
	int displayw,
	int displayh,
	ContentManager & content,
	FONT & font)
{
	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	std::tr1::shared_ptr<TEXTURE> boxtex, bartex;
	if (!content.load(texturepath, "loadingbox.png", texinfo, boxtex)) return false;
	if (!content.load(texturepath, "loadingbar.png", texinfo, bartex)) return false;

	bardraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	boxdraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	barbackdraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & bardrawref = root.GetDrawlist().twodim.get(bardraw);
	DRAWABLE & boxdrawref = root.GetDrawlist().twodim.get(boxdraw);
	DRAWABLE & barbackdrawref = root.GetDrawlist().twodim.get(barbackdraw);

	boxdrawref.SetDiffuseMap(boxtex);
	boxdrawref.SetVertArray(&boxverts);
	boxdrawref.SetDrawOrder(10000);
	boxdrawref.SetCull(false, false);
	boxdrawref.SetColor(1,1,1,1);

	w = 128.0/displayw*3.;
	h = 128.0/displayw;

	barbackdrawref.SetDiffuseMap(bartex);
	barbackdrawref.SetVertArray(&barbackverts);
	barbackdrawref.SetDrawOrder(10001);
	barbackdrawref.SetCull(false, false);
	barbackdrawref.SetColor(0.3, 0.3, 0.3, 0.4);

	hscale = 0.3;

	bardrawref.SetDiffuseMap(bartex);
	bardrawref.SetVertArray(&barverts);
	bardrawref.SetDrawOrder(10002);
	bardrawref.SetCull(false, false);
	bardrawref.SetColor(1,1,1, 0.7);

	float screenhwratio = displayh/(float)displayw;
	float fontscaley = 0.02;
	float fontscalex = fontscaley*screenhwratio;
	text.Init(root, font, "", -w*0.5, -0.5*h*hscale, fontscalex, fontscaley);
	text.SetDrawOrder(root, 10003);

	Update(0.0,"",0.5,0.5);

	return true;
}
