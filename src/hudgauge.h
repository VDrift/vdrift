#ifndef _HUDGAUGE_H
#define _HUDGAUGE_H

#include "scenenode.h"
#include <ostream>
#include <string>

class FONT;

class HUDGAUGE
{
public:
	HUDGAUGE();

	// startangle and endangle in rad
	// startvalue + endvalue = n * valuedelta
	void Set(
		SCENENODE & parent,
		const FONT & font,
		const std::string & name,
		float hwratio,
		float centerx,
		float centery,
		float radius,
		float startangle,
		float endangle,
		float startvalue,
		float endvalue,
		float valuedelta);

	void Update(SCENENODE & parent, float value);

private:
	keyed_container<SCENENODE>::handle pointer_node;
	keyed_container<DRAWABLE>::handle pointer_draw;
	keyed_container<DRAWABLE>::handle dialnum_draw;
	keyed_container<DRAWABLE>::handle dial_draw;
	VERTEXARRAY pointer_rotated;
	VERTEXARRAY pointer;
	VERTEXARRAY dial_label;
	VERTEXARRAY dial_marks;
	float centerx;
	float centery;
	float scalex;
	float scaley;
	float offset;
	float scale;
};

#endif // _HUDGAUGE_H
