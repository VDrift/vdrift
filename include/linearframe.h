#ifndef _LINEARFRAME_H
#define _LINEARFRAME_H

#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"

//#define EULER
//#define NSV
//#define VELOCITYVERLET
#define SUVAT

template <typename T>
class LINEARFRAME
{
friend class joeserialize::Serializer;
public:
	LINEARFRAME() :
		force(0),
		inverse_mass(1.0),
		halfstep(false)
	{
	}

	const MATHVECTOR <T, 3> & GetForce() const
	{
		return force;
	}

	const MATHVECTOR <T, 3> GetVelocity() const
	{
		return GetVelocityFromMomentum(momentum);
	}

	const MATHVECTOR <T, 3> & GetPosition() const
	{
		return position;
	}

	const T GetMass() const
	{
		return 1.0 / inverse_mass;
	}

	const T GetInvMass() const
	{
		return inverse_mass;
	}

	/// this must only be called between integrate1 and integrate2 steps
	void ApplyForce(const MATHVECTOR <T, 3> & f)
	{
		assert(halfstep);
		force = force + f;
	}

	/// this must only be called between integrate1 and integrate2 steps
	void SetForce(const MATHVECTOR <T, 3> & f)
	{
		assert(halfstep);
		force = f;
	}

	void SetVelocity(const MATHVECTOR <T, 3> & velocity)
	{
		momentum = velocity / inverse_mass;
	}

	void SetPosition(const MATHVECTOR <T, 3> & newpos)
	{
		position = newpos;
	}

	void SetMass(const T & mass)
	{
		inverse_mass = 1.0 / mass;
	}

	/// both steps must be executed each frame and forces can only be set between steps 1 and 2
	void Integrate1(const T & dt)
	{
		assert(!halfstep);

#ifdef VELOCITYVERLET
		momentum = momentum + force * dt * 0.5;
		position = position + momentum * inverse_mass * dt;
#endif

		force.Set(0.0);
		halfstep = true;
	}

	/// both steps must be executed each frame and forces must be set between steps 1 and 2
	void Integrate2(const T & dt)
	{
		assert(halfstep);

#ifdef VELOCITYVERLET
		momentum = momentum + force * dt * 0.5;
#endif

#ifdef NSV
		momentum = momentum + force * dt;
		position = position + momentum * inverse_mass * dt;
#endif

#ifdef EULER
		position = position + momentum * inverse_mass * dt;
		momentum = momentum + force * dt;
#endif

#ifdef SUVAT
		position = position + momentum * inverse_mass * dt + force * inverse_mass * dt * dt * 0.5;
		momentum = momentum + force * dt;
#endif

		halfstep = false;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, position);
		_SERIALIZE_(s, momentum);
		_SERIALIZE_(s, force);
		return true;
	}

private:
	//primary
	MATHVECTOR <T, 3> position;
	MATHVECTOR <T, 3> momentum;
	MATHVECTOR <T, 3> force;

	//constants
	T inverse_mass;

	//housekeeping
	bool halfstep;

	MATHVECTOR <T, 3> GetVelocityFromMomentum(const MATHVECTOR <T, 3> & moment) const
	{
		return moment * inverse_mass;
	}
};

#endif
