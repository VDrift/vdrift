/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _SIM_MOTORJOINT_H
#define _SIM_MOTORJOINT_H

#include "shaft.h"

namespace sim
{

/// target velocity determines whether the impulse limit is accelerating or decelerating
struct MotorJoint
{
	Shaft * shaft;
	btScalar impulseLimit;
	btScalar targetVelocity;
	btScalar accumulatedImpulse;

	/// reset solver
	void init();

	/// one solver iteration
	void solve();
};

// implementation

inline void MotorJoint::init()
{
	accumulatedImpulse = 0;
}

inline void MotorJoint::solve()
{
	btScalar velocityError = shaft->getAngularVelocity() - targetVelocity;
	btScalar lambda = -velocityError * shaft->getInertia();

	btScalar accumulatedImpulseOld = accumulatedImpulse;
	accumulatedImpulse += lambda;
	btClamp(accumulatedImpulse, -impulseLimit, impulseLimit);
	lambda = accumulatedImpulse - accumulatedImpulseOld;

	shaft->applyImpulse(lambda);
}

}

#endif
