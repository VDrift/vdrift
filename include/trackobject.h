#ifndef _TRACK_OBJECT_H
#define _TRACK_OBJECT_H

#include "mathvector.h"
#include "quaternion.h"

class MODEL;
class TRACKSURFACE;

struct TRACKOBJECT
{
	const MODEL * model;
	const TRACKSURFACE * surface;
	MATHVECTOR<float, 3> position;
	QUATERNION<float> rotation;
	
	TRACKOBJECT() : model(0), surface(0) { }
};

#endif
