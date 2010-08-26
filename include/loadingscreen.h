#ifndef _LOADINGSCREEN_H
#define _LOADINGSCREEN_H

#include "scenenode.h"
#include "vertexarray.h"

#include <ostream>
#include <string>

template <class T, class Tinfo> class MANAGER;
class TEXTURE;
class TEXTUREINFO;

class LOADINGSCREEN
{
public:
	SCENENODE & GetNode() {return root;}
	
	void Update(float percentage);

	///initialize the loading screen given the root node for the loading screen
	bool Init(
		const std::string & texturepath,
		int displayw,
		int displayh,
		const std::string & texsize,
		MANAGER<TEXTURE, TEXTUREINFO> & textures);

private:
	SCENENODE root;
	keyed_container <DRAWABLE>::handle bardraw;
	VERTEXARRAY barverts;
	keyed_container <DRAWABLE>::handle barbackdraw;
	VERTEXARRAY barbackverts;
	keyed_container <DRAWABLE>::handle boxdraw;
	VERTEXARRAY boxverts;
	float w, h, hscale;
};

#endif
