#ifndef _TRACKOBJECT_H
#define _TRACKOBJECT_H

#include "modelptr.h"

#include <cassert>

class TRACKSURFACE;

class TrackObject
{
public:
	TrackObject(ModelPtr m, const TRACKSURFACE * s) :
		model(m),
		surface(s)
	{
		assert(model.get());
	}
	
	const MODEL & getModel() const
	{
		return *model;
	}

	const TRACKSURFACE * getSurface() const
	{
		return surface;
	}

private:
	ModelPtr model;
	const TRACKSURFACE * surface;
};

#endif
