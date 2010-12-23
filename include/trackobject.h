#ifndef _TRACK_OBJECT_H
#define _TRACK_OBJECT_H

class MODEL;
class TRACKSURFACE;

struct TRACKOBJECT
{
	const MODEL * model;
	const TRACKSURFACE * surface;
	
	TRACKOBJECT() : model(0), surface(0) {}
	
	TRACKOBJECT(const MODEL * model, const TRACKSURFACE * surface) : model(model), surface(surface) {}
};

#endif
