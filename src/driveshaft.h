#ifndef _DRIVESHAFT_H
#define _DRIVESHAFT_H

#include "LinearMath/btScalar.h"

struct DriveShaft
{
	btScalar inv_inertia;
	btScalar ang_velocity;
	btScalar angle;

	DriveShaft() : inv_inertia(1), ang_velocity(0), angle(0) {}

	// amount of momentum to reach angular velocity
	btScalar getMomentum(btScalar angvel) const
	{
		return (angvel - ang_velocity) / inv_inertia;
	}

	// update angular velocity
	void applyMomentum(btScalar momentum)
	{
		ang_velocity += inv_inertia * momentum;
	}

	// update angle
	void integrate(btScalar dt)
	{
		angle += ang_velocity * dt;
	}
};

#endif // _DRIVESHAFT_H
