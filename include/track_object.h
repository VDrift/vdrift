#ifndef _TRACK_OBJECT_H
#define _TRACK_OBJECT_H

#include "assert.h"

class MODEL;
class TRACKSURFACE;

class TRACK_OBJECT
{
public:
	TRACK_OBJECT(MODEL * m, const TRACKSURFACE * s) 
	: model(m), surface(s)
	{
		assert(model);
	}
	
	MODEL * GetModel() const
	{
		return model;
	}

	const TRACKSURFACE * GetSurface() const
	{
		return surface;
	}

private:
	MODEL * model;
	const TRACKSURFACE * surface;
};

#endif
