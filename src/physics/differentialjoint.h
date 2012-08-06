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

#ifndef _SIM_DIFFERENTIALJOINT_H
#define _SIM_DIFFERENTIALJOINT_H

#include "shaft.h"

namespace sim
{

/// shafts are co-rotating to simplify joint chains
struct DifferentialJoint
{
	Shaft * shaft1;
	Shaft * shaft2a;
	Shaft * shaft2b;
	btScalar gearRatio;
	btScalar inertiaEff;
	btScalar impulseLimit;
	btScalar accumulatedImpulse;
	
	/// calculate effective inertia, reset solver variables
	void init();
	
	/// one solver iteration
	void solve();
};

// implementation

inline void DifferentialJoint::init()
{
	accumulatedImpulse = 0;
	impulseLimit = SIMD_INFINITY;
	inertiaEff = 1 / (shaft1->getInertiaInv() +
		gearRatio * gearRatio / (shaft2a->getInertia() + shaft2b->getInertia()));
}

inline void DifferentialJoint::solve()
{
	btScalar velocityError = shaft1->getAngularVelocity() -
		0.5 * gearRatio * (shaft2a->getAngularVelocity() + shaft2b->getAngularVelocity());
	btScalar lambda = -velocityError * inertiaEff;

	btScalar accumulatedImpulseOld = accumulatedImpulse;
	accumulatedImpulse += lambda;
	btClamp(accumulatedImpulse, -impulseLimit, impulseLimit);
	lambda = accumulatedImpulse - accumulatedImpulseOld;

	shaft1->applyImpulse(lambda);
	shaft2a->applyImpulse(-lambda * 0.5 * gearRatio);
	shaft2b->applyImpulse(-lambda * 0.5 * gearRatio);
}

}

#endif
