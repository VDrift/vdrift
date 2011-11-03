#ifndef _HUDGAUGE_H
#define _HUDGAUGE_H

#include "scenenode.h"
#include <ostream>

class FONT;

class HUDGAUGE
{
public:
	HUDGAUGE();

	// startangle and endangle in rad
	// startvalue + endvalue = numvalues * delta
	void Set(
		SCENENODE & parent,
		FONT & font,
		float hwratio,
		float centerx,
		float centery,
		float radius,
		float startangle,
		float endangle,
		float startvalue,
		float endvalue,
		int numvalues,
		std::ostream & error_output);

	void Update(SCENENODE & parent, float value);

private:
	keyed_container<SCENENODE>::handle pointer_node;
	VERTEXARRAY pointer_rotated;
	VERTEXARRAY pointer;
	VERTEXARRAY dialnum;
	VERTEXARRAY dial;
	float centerx;
	float centery;
	float scalex;
	float scaley;
	float offset;
	float scale;
};

#endif // _HUDGAUGE_H
