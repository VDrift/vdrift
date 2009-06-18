#include <string>
using std::string;

#ifndef _TRACKSURFACETYPE_H
#define _TRACKSURFACETYPE_H

struct TRACKSURFACE
{
	int surface;
	string surfaceName;
	float bumpWaveLength;
	float bumpAmplitude;
	float frictionNonTread;
	float frictionTread;
	float rollResistanceCoefficient;
	float rollingDrag;
};

#endif

