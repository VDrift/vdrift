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

#ifndef _SIM_SOLVECONSTRAINTROW_H
#define _SIM_SOLVECONSTRAINTROW_H

#include "constraintrow.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

namespace sim
{

inline void SolveConstraintRow(
	ConstraintRow & constraint,
	btRigidBody & bodyA,
	btRigidBody & bodyB,
	btVector3 & rA,
	btVector3 & rB,
	btScalar velocityError = 0)
{
	btVector3 normal = constraint.normal;
	btVector3 dVA = bodyA.getLinearVelocity() + bodyA.getAngularVelocity().cross(rA);
	btVector3 dVB = bodyB.getLinearVelocity() + bodyB.getAngularVelocity().cross(rB);
	velocityError += normal.dot(dVA - dVB);
	btScalar deltaImpulse = constraint.rhs + constraint.cfm * constraint.accumImpulse - velocityError * constraint.jacDiagInv;

	btScalar oldImpulse = constraint.accumImpulse;
	constraint.accumImpulse = oldImpulse + deltaImpulse;
	btClamp(constraint.accumImpulse, constraint.lowerLimit, constraint.upperLimit);
	deltaImpulse = constraint.accumImpulse - oldImpulse;

	bodyA.setLinearVelocity(bodyA.getLinearVelocity() + deltaImpulse * bodyA.internalGetInvMass() * normal);
	bodyA.setAngularVelocity(bodyA.getAngularVelocity() + deltaImpulse * constraint.angularCompA);
	bodyB.setLinearVelocity(bodyB.getLinearVelocity() - deltaImpulse * bodyB.internalGetInvMass() * normal);
	bodyB.setAngularVelocity(bodyB.getAngularVelocity() - deltaImpulse * constraint.angularCompB);
}

}

#endif
