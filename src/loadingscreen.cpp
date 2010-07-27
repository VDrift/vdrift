#include "loadingscreen.h"

#include "contentmanager.h"
#include "textureloader.h"

void LOADINGSCREEN::Update(float percentage)
{
	if (percentage < 0)
		percentage = 0;
	if (percentage > 1.0)
		percentage = 1.0;
	
	barverts.SetToBillboard(0.5-w*0.5,0.5-h*0.5*hscale,0.5-w*0.5+w*percentage, 0.5+h*0.5*hscale);
}

///initialize the loading screen given the root node for the loading screen
bool LOADINGSCREEN::Init(
	const std::string & texturepath,
	int displayw,
	int displayh,
	const std::string & texsize,
	ContentManager & content)
{
	TextureLoader texload;
	texload.mipmap = false;
	texload.size = texsize;
	
	texload.name = texturepath + "/loadingbox.png";
	TexturePtr boxtex = content.get<TEXTURE>(texload);
	if (!boxtex.get()) return false;
	
	texload.name = texturepath + "/loadingbar.png";
	TexturePtr bartex = content.get<TEXTURE>(texload);
	if (!bartex.get()) return false;
	
	bardraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	boxdraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	barbackdraw = root.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & bardrawref = root.GetDrawlist().twodim.get(bardraw);
	DRAWABLE & boxdrawref = root.GetDrawlist().twodim.get(boxdraw);
	DRAWABLE & barbackdrawref = root.GetDrawlist().twodim.get(barbackdraw);
	
	boxdrawref.SetDiffuseMap(boxtex);
	boxdrawref.SetVertArray(&boxverts);
	boxdrawref.SetDrawOrder(0);
	boxdrawref.SetCull(false, false);
	boxdrawref.SetColor(1,1,1,1);
	
	w = 128.0/displayw;
	h = 128.0/displayw;
	boxverts.SetTo2DButton(0.5,0.5,w,h,w*0.5,false);
	
	barbackdrawref.SetDiffuseMap(bartex);
	barbackdrawref.SetVertArray(&barbackverts);
	barbackdrawref.SetDrawOrder(1);
	barbackdrawref.SetCull(false, false);
	barbackdrawref.SetColor(0.3, 0.3, 0.3, 0.4);
	
	hscale = 0.3;
	barbackverts.SetToBillboard(0.5-w*0.5,0.5-h*0.5*hscale,0.5+w*0.5, 0.5+h*0.5*hscale);
	
	bardrawref.SetDiffuseMap(bartex);
	bardrawref.SetVertArray(&barverts);
	bardrawref.SetDrawOrder(2);
	bardrawref.SetCull(false, false);
	bardrawref.SetColor(1,1,1, 0.7);
	
	return true;
}
