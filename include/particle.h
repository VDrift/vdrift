#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "mathvector.h"
#include "quaternion.h"
#include "scenenode.h"
#include "vertexarray.h"
#include "textureptr.h"

class Particle
{
public:
	Particle(
		SCENENODE & parentnode,
		const MATHVECTOR <float, 3> & new_start_position,
		const MATHVECTOR <float, 3> & new_dir,
		float newspeed,
		float newtrans,
		float newlong,
		float newsize,
		TexturePtr texture);

	Particle(const Particle & other)
	{
		Set(other);
	}

	Particle & operator=(const Particle & other)
	{
		Set(other);
		return *this;
	}

	keyed_container <SCENENODE>::handle & GetNode()
	{
		return node;
	}

	bool Expired() const
	{
		return (time > longevity);
	}

	void Update(
		SCENENODE & parent,
		float dt,
		const QUATERNION <float> & camdir_conjugate,
		const MATHVECTOR <float, 3> & campos);

private:
	MATHVECTOR <float, 3> start_position;
	MATHVECTOR <float, 3> direction;
	float speed;
	float transparency;
	float longevity;
	float size;
	float time; ///< time since the particle was created; i.e. the particle's age

	keyed_container <SCENENODE>::handle node;
	keyed_container <DRAWABLE>::handle draw;
	VERTEXARRAY varray;

	static keyed_container <DRAWABLE> & GetDrawlist(SCENENODE & node)
	{
		return node.GetDrawlist().particle;
	}

	void Set(const Particle & other)
	{
		transparency = other.transparency;
		longevity = other.longevity;
		start_position = other.start_position;
		speed = other.speed;
		direction = other.direction;
		size = other.size;
		time = other.time;
		node = other.node;
		draw = other.draw;
		varray = other.varray;
	}
};

#endif
