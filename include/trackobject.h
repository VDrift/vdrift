#ifndef _TRACK_OBJECT_H
#define _TRACK_OBJECT_H

#include <cassert>

class MODEL;
class TRACKSURFACE;

class TRACKOBJECT
{
public:
	TRACKOBJECT(std::tr1::shared_ptr<MODEL_JOE03> model, const TRACKSURFACE * s) :
		model(model), surface(s)
	{
		assert(model.get());
	}
	
	const MODEL * GetModel() const
	{
		return model.get();
	}

	const TRACKSURFACE * GetSurface() const
	{
		return surface;
	}

private:
	std::tr1::shared_ptr<MODEL_JOE03> model;
	const TRACKSURFACE * surface;
};

#endif
