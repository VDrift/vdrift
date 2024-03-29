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

#ifndef _WHEEL_CONSTRAINT_H
#define _WHEEL_CONSTRAINT_H

#include "minmax.h"
#include "driveshaft.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

struct ConstraintRow
{
	btVector3 axis;
	btVector3 inv_inertia;
	btScalar mass;
	btScalar rhs;
	btScalar cfm;
	btScalar impulse;
	btScalar lower_impulse_limit;
	btScalar upper_impulse_limit;

	// update constraint impulse and return correction impulse
	btScalar solve(btScalar velocity_error)
	{
		btScalar impulse_delta = rhs + cfm * impulse - velocity_error * mass;
		btScalar impulse_old = impulse;
		impulse = Clamp(impulse + impulse_delta, lower_impulse_limit, upper_impulse_limit);
		return impulse - impulse_old;
	}

	// ignore rhs and cfm softening parameters
	btScalar solveHard(btScalar velocity_error)
	{
		btScalar impulse_delta = -velocity_error * mass;
		btScalar impulse_old = impulse;
		impulse = Clamp(impulse + impulse_delta, lower_impulse_limit, upper_impulse_limit);
		return impulse - impulse_old;
	}

	void initMassInertia(const btRigidBody & body, const btVector3 & pos, btScalar softness = 0)
	{
		btVector3 c = pos.cross(axis);
		inv_inertia = body.getInvInertiaTensorWorld() * c;

		btVector3 v = (c * body.getInvInertiaTensorWorld()).cross(pos);
		mass = 1 / (body.getInvMass() + axis.dot(v) + softness);
	}
};

struct WheelConstraint
{
	DriveShaft * shaft;
	ConstraintRow constraint[3];
	btVector3 position;
	btScalar radius;
	btScalar vcam;

	void init(const btRigidBody & body, btScalar softness, btScalar error)
	{
		constraint[0].initMassInertia(body, position);
		constraint[0].mass = constraint[0].mass * shaft->inertia /
			(constraint[0].mass * radius * radius + shaft->inertia);
		constraint[0].impulse = 0;

		constraint[1].initMassInertia(body, position);
		constraint[1].impulse = 0;

		constraint[2].initMassInertia(body, position, softness);
		constraint[2].rhs = -error * constraint[2].mass;
		constraint[2].cfm = -softness * constraint[2].mass;
		constraint[2].lower_impulse_limit = 0;
		constraint[2].upper_impulse_limit = error < 0 ? SIMD_INFINITY : 0;
		constraint[2].impulse = 0;
	}

	void init(const btRigidBody & body, btScalar stiffness, btScalar damping, btScalar displacement, btScalar dt)
	{
		btScalar softness = 1 / (dt * (dt * stiffness + damping));
		btScalar bias = stiffness / (dt * stiffness + damping);
		btScalar error = -bias * displacement;
		init(body, softness, error);
	}

	void getContactVelocity(const btRigidBody & body, btScalar v[3]) const
	{
		btVector3 vb = body.getVelocityInLocalPoint(position);
		v[0] = constraint[0].axis.dot(vb);
		v[1] = constraint[1].axis.dot(vb);
		v[2] = shaft->ang_velocity * radius;
	}

	void solveFriction(btRigidBody & body)
	{
		btVector3 vb = body.getVelocityInLocalPoint(position);
		btScalar ve = constraint[0].axis.dot(vb) - shaft->ang_velocity * radius;
		btScalar dp = constraint[0].solveHard(ve);
		btVector3 dw = constraint[0].inv_inertia * dp;
		btVector3 dv = constraint[0].axis * (dp * body.getInvMass());
		body.setAngularVelocity(body.getAngularVelocity() + dw);
		body.setLinearVelocity(body.getLinearVelocity() + dv);
		shaft->applyImpulse(-dp * radius);

		vb = body.getVelocityInLocalPoint(position);
		ve = constraint[1].axis.dot(vb) - vcam;
		dp = constraint[1].solveHard(ve);
		dw = constraint[1].inv_inertia * dp;
		dv = constraint[1].axis * (dp * body.getInvMass());
		body.setAngularVelocity(body.getAngularVelocity() + dw);
		body.setLinearVelocity(body.getLinearVelocity() + dv);
	}

	void solveSuspension(btRigidBody & body)
	{
		btVector3 vb = body.getVelocityInLocalPoint(position);
		btScalar ve = constraint[2].axis.dot(vb);
		btScalar dp = constraint[2].solve(ve);
		btVector3 dw = constraint[2].inv_inertia * dp;
		btVector3 dv = constraint[2].axis * (dp * body.getInvMass());
		body.setAngularVelocity(body.getAngularVelocity() + dw);
		body.setLinearVelocity(body.getLinearVelocity() + dv);
	}
};

#endif // _WHEEL_CONSTRAINT_H

