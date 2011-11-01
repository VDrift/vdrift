#ifndef _HUDBAR_H
#define _HUDBAR_H

#include "vertexarray.h"
#include "drawable.h"

class SCENENODE;
class TEXTURE;

class HUDBAR
{
public:
	void Set(
		SCENENODE & parent,
		std::tr1::shared_ptr<TEXTURE> bartex,
		float x, float y, float w, float h,
		float opacity,
		bool flip);

	void SetVisible(SCENENODE & parent, bool newvis);

private:
	keyed_container<DRAWABLE>::handle draw;
	VERTEXARRAY verts;
};

#endif // _HUDBAR_H
