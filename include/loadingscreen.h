#ifndef _LOADINGSCREEN_H
#define _LOADINGSCREEN_H

#include "texture.h"
#include "scenegraph.h"
#include "vertexarray.h"

#include <ostream>
#include <string>

class LOADINGSCREEN
{
private:
	SCENENODE * root;
	TEXTURE_GL bartex;
	DRAWABLE * bardraw;
	VERTEXARRAY barverts;
	DRAWABLE * barbackdraw;
	VERTEXARRAY barbackverts;
	TEXTURE_GL boxtex;
	DRAWABLE * boxdraw;
	VERTEXARRAY boxverts;
	float w, h, hscale;
	
public:
	LOADINGSCREEN() : root(NULL),bardraw(NULL),boxdraw(NULL) {}
	
	///initialize the loading screen given the root node for the loading screen
	bool Initialize(SCENENODE & rootnode, const std::string & texturepath, int displayw, int displayh, const std::string & texsize, std::ostream & error_output)
	{
		TEXTUREINFO boxtexinfo(texturepath+"/loadingbox.png");
		boxtexinfo.SetMipMap(false);
		if (!boxtex.Load(boxtexinfo, error_output, texsize))
		{
			error_output << "Error loading graphic for loading screen." << std::endl;
			return false;
		}
		
		TEXTUREINFO bartexinfo(texturepath+"/loadingbar.png");
		bartexinfo.SetMipMap(false);
		if (!bartex.Load(bartexinfo, error_output, texsize))
		{
			error_output << "Error loading graphic for loading screen." << std::endl;
			return false;
		}
		
		root = &rootnode;
		bardraw = &root->AddDrawable();
		assert(bardraw);
		boxdraw = &root->AddDrawable();
		assert(boxdraw);
		barbackdraw = &root->AddDrawable();
		assert(barbackdraw);
		
		boxdraw->SetDiffuseMap(&boxtex);
		boxdraw->SetVertArray(&boxverts);
		boxdraw->SetDrawOrder(0);
		boxdraw->SetLit(false);
		boxdraw->Set2D(true);
		boxdraw->SetCull(false, false);
		boxdraw->SetColor(1,1,1,1);
		
		w = 128.0/displayw;
		h = 128.0/displayw;
		boxverts.SetTo2DButton(0.5,0.5,w,h,w*0.5,false);
		
		barbackdraw->SetDiffuseMap(&bartex);
		barbackdraw->SetVertArray(&barbackverts);
		barbackdraw->SetDrawOrder(1);
		barbackdraw->SetLit(false);
		barbackdraw->Set2D(true);
		barbackdraw->SetCull(false, false);
		//barbackdraw->SetColor(0.3, 0.3, 0.3, 0.4);
		barbackdraw->SetColor(0.3, 0.3, 0.3, 0.4);
		
		hscale = 0.3;
		barbackverts.SetToBillboard(0.5-w*0.5,0.5-h*0.5*hscale,0.5+w*0.5, 0.5+h*0.5*hscale);
		
		bardraw->SetDiffuseMap(&bartex);
		bardraw->SetVertArray(&barverts);
		bardraw->SetDrawOrder(2);
		bardraw->SetLit(false);
		bardraw->Set2D(true);
		bardraw->SetCull(false, false);
		//bardraw->SetColor(0.3, 0.3, 0.3, 0.4);
		bardraw->SetColor(1,1,1, 0.7);
		
		return true;
	}
	
	void Update(float percentage)
	{
		//assert(percentage >= 0 && percentage <= 1.0);
		if (percentage < 0)
			percentage = 0;
		if (percentage > 1.0)
			percentage = 1.0;
		assert(root);
		assert(bardraw);
		assert(boxdraw);
		
		barverts.SetToBillboard(0.5-w*0.5,0.5-h*0.5*hscale,0.5-w*0.5+w*percentage, 0.5+h*0.5*hscale);
	}
};

#endif
