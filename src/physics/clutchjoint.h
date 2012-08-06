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

#ifndef _SIM_CLUTCHJOINT_H
#define _SIM_CLUTCHJOINT_H

#include "shaft.h"

namespace sim
{

/// shaft 1 and 2 are co-rotating
/// gearRatio has to remain unequal to 0 at any time
struct ClutchJoint
{
	Shaft * shaft1;
	Shaft * shaft2;
	btScalar gearRatio;
	btScalar impulseLimit;
	btScalar inertiaEff;
	btScalar accumulatedImpulse;
	
	/// calculate effective inertia, reset impulse accuulator
	void init();
	
	/// one solver ieration
	void solve();
};

// implementation

inline void ClutchJoint::init()
{
	accumulatedImpulse = 0;
	inertiaEff = 1.0 / (shaft1->getInertiaInv() +
		gearRatio * gearRatio * shaft2->getInertiaInv());
}

inline void ClutchJoint::solve()
{
	btScalar velocityError = shaft1->getAngularVelocity() - gearRatio * shaft2->getAngularVelocity();
	btScalar lambda = -velocityError * inertiaEff;

	btScalar accumulatedImpulseOld = accumulatedImpulse;
	accumulatedImpulse += lambda;
	btClamp(accumulatedImpulse, -impulseLimit, impulseLimit);
	lambda = accumulatedImpulse - accumulatedImpulseOld;

	shaft1->applyImpulse(lambda);
	shaft2->applyImpulse(-lambda * gearRatio);
}

}

#endif
