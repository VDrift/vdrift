#ifndef _LOADINGSCREEN_H
#define _LOADINGSCREEN_H

#include "scenenode.h"
#include "vertexarray.h"
#include "text_draw.h"

#include <ostream>
#include <string>

class ContentManager;

class LOADINGSCREEN
{
public:
	SCENENODE & GetNode() {return root;}

	void Update(float percentage, const std::string & optional_text, float posx, float posy);

	///initialize the loading screen given the root node for the loading screen
	bool Init(
		const std::string & texturepath,
		int displayw,
		int displayh,
		ContentManager & content,
		FONT & font);

private:
	SCENENODE root;
	keyed_container <DRAWABLE>::handle bardraw;
	VERTEXARRAY barverts;
	keyed_container <DRAWABLE>::handle barbackdraw;
	VERTEXARRAY barbackverts;
	keyed_container <DRAWABLE>::handle boxdraw;
	VERTEXARRAY boxverts;
	float w, h, hscale;
	TEXT_DRAWABLE text;
};

#endif
