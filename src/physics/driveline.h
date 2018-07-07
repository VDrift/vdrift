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

#ifndef _DRIVELINE_H
#define _DRIVELINE_H

#include "minmax.h"
#include "driveshaft.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

// target velocity determines whether the impulse limit is accelerating or decelerating
struct MotorJoint
{
	DriveShaft * shaft;
	btVector3 inv_body_inertia;
	btScalar joint_inertia;
	btScalar target_velocity;
	btScalar impulse_limit_delta;
	btScalar impulse_limit;
	btScalar impulse;

	MotorJoint() :
		shaft(0),
		inv_body_inertia(0,0,0),
		joint_inertia(0),
		target_velocity(0),
		impulse_limit_delta(0),
		impulse_limit(0),
		impulse(0)
	{
		// ctor
	}

	void computeInertia(btRigidBody & body, const btVector3 & shaft_axis)
	{
		inv_body_inertia = body.getInvInertiaTensorWorld() * shaft_axis;
		joint_inertia = 1 / (body.computeAngularImpulseDenominator(shaft_axis) + shaft->inv_inertia);
	}

	void solve(btRigidBody & body)
	{
		btScalar velocity_error = shaft->ang_velocity - target_velocity;
		btScalar lambda = -velocity_error * joint_inertia;
		btScalar impulse_old = impulse;
		impulse = Clamp(impulse + lambda, -impulse_limit, impulse_limit);
		lambda = impulse - impulse_old;
		shaft->applyImpulse(lambda);
		body.setAngularVelocity(body.getAngularVelocity() + inv_body_inertia * lambda);
	}
};

struct ClutchJoint
{
	btScalar inertia;
	btScalar softness;		// slip speed sensitivity, from no damping to max damping(lock): [0, 1]
	btScalar load_coeff;	// clutch driveshaft torque sensitivity
	btScalar decel_factor;	// 0 lock only when accelerating, 1 lock always
	btScalar impulse_limit_delta;	// impulse limit interpolation value
	btScalar impulse_limit;	// speed sensitive clutch limit
	btScalar impulse;		// accumulated clutch impulse

	ClutchJoint() :
		inertia(0),
		softness(0),
		load_coeff(0),
		decel_factor(0),
		impulse_limit_delta(0),
		impulse_limit(0),
		impulse(0)
	{
		// ctor
	}

	// returns clamped velocity error correction impulse
	btScalar solve(btScalar velocity_error)
	{
		btScalar lambda = -velocity_error * inertia;
		btScalar impulse_old = impulse;
		impulse = Clamp(impulse + lambda, -impulse_limit, impulse_limit);
		return impulse - impulse_old;
	}

	// applies clamped velocity error correction impulse
	void solve(DriveShaft & sa, DriveShaft & sb)
	{
		btScalar velocity_error = sa.ang_velocity - sb.ang_velocity;
		btScalar lambda = solve(velocity_error);
		sa.applyImpulse(lambda);
		sb.applyImpulse(-lambda);
	}

	// more complex variant that takes clutch load and softness into account
	// returns clamped velocity error correction impulse
	btScalar solve(btScalar velocity_error, btScalar load)
	{
		btScalar limit = impulse_limit + load_coeff * load;
		btScalar lambda = -(velocity_error + softness * impulse) * inertia;
		btScalar impulse_old = impulse;
		impulse = Clamp(impulse + lambda, -limit, limit);
		return impulse - impulse_old;
	}

	// more complex variant that takes clutch load and softness into account
	// applies clamped velocity error correction impulse
	void solve(DriveShaft & sa, DriveShaft & sb, btScalar load)
	{
		btScalar velocity_error = sa.ang_velocity - sb.ang_velocity;
		btScalar lambda = solve(velocity_error, load);
		sa.applyImpulse(lambda);
		sb.applyImpulse(-lambda);
	}
};

// 2wd driveline methods use suffix 2
// 4wd driveline methods use suffix 4
struct Driveline
{
	DriveShaft * shaft[5];	// engine and wheels
	MotorJoint motor[5];	// engine and brakes
	ClutchJoint clutch[4];	// transmission and differential clutches
	btScalar gear_ratio;	// transmission * differential gear ratio
	btScalar torque_split;	// center differential torque split
	unsigned motor_count;	// active motor joints
	unsigned clutch_count;	// active clutch joints

	Driveline() : gear_ratio(0), torque_split(0), motor_count(0), clutch_count(0) {}

	void clearImpulses()
	{
		for (unsigned i = 0; i < motor_count; ++i)
			motor[i].impulse = 0;

		for (unsigned i = 0; i < clutch_count; ++i)
			clutch[i].impulse = 0;
	}

	void updateImpulseLimits()
	{
		for (unsigned i = 0; i < motor_count; ++i)
			motor[i].impulse_limit += motor[i].impulse_limit_delta;

		for (unsigned i = 0; i < clutch_count; ++i)
			clutch[i].impulse_limit += clutch[i].impulse_limit_delta;
	}

	void computeInertia2()
	{
		computeDiffInertia2();
		computeDriveInertia2();
	}

	void computeDriveInertia2()
	{
		btScalar n1 = 1 / (shaft[1]->inertia + shaft[2]->inertia);
		btScalar i0 = 1 / (shaft[0]->inv_inertia + gear_ratio * gear_ratio * n1);
		clutch[0].inertia = i0;
	}

	void computeDiffInertia2()
	{
		btScalar i1 = 1 / (shaft[1]->inv_inertia + shaft[2]->inv_inertia + clutch[1].softness);
		clutch[1].inertia = i1;
	}

	void solve2(btRigidBody & body)
	{
		solveMotors(body);
		solveDiff2();
		solveDiffClutch2();
	}

	void solveMotors(btRigidBody & body)
	{
		for (unsigned i = 0; i < motor_count; ++i)
			motor[i].solve(body);
	}

	void solveDiff2()
	{
		btScalar velocity = btScalar(0.5) * (shaft[1]->ang_velocity + shaft[2]->ang_velocity);
		btScalar velocity_error = shaft[0]->ang_velocity - velocity * gear_ratio;
		btScalar impulse = clutch[0].solve(velocity_error);
		btScalar impulse2 = btScalar(-0.5) * (impulse * gear_ratio);
		shaft[0]->applyImpulse(impulse);
		shaft[1]->applyImpulse(impulse2);
		shaft[2]->applyImpulse(impulse2);
	}

	void solveDiffClutch2()
	{
		btScalar load = Max(-clutch[0].impulse, clutch[0].impulse * clutch[1].decel_factor);
		if (load > 0)
			clutch[1].solve(*shaft[1], *shaft[2], load);
	}

	void computeInertia4()
	{
		computeDiffInertia4();
		computeDriveInertia4();
	}

	void computeDriveInertia4()
	{
		btScalar n1 = 1 / (shaft[3]->inertia + shaft[4]->inertia +
						   shaft[1]->inertia + shaft[2]->inertia);
		btScalar i0 = 1 / (shaft[0]->inv_inertia + gear_ratio * gear_ratio * n1);
		clutch[0].inertia = i0;
	}

	void computeDiffInertia4()
	{
		btScalar i3 = 1 / (shaft[3]->inv_inertia + shaft[4]->inv_inertia + clutch[3].softness);
		btScalar i2 = 1 / (shaft[1]->inv_inertia + shaft[2]->inv_inertia + clutch[2].softness);
		btScalar n3 = 1 / (shaft[3]->inertia + shaft[4]->inertia);
		btScalar n2 = 1 / (shaft[1]->inertia + shaft[2]->inertia);
		btScalar i1 = 1 / (n2 + n3 + clutch[1].softness);
		clutch[3].inertia = i3;
		clutch[2].inertia = i2;
		clutch[1].inertia = i1;
	}

	void solve4(btRigidBody & body)
	{
		solveMotors(body);
		solveDiff4();
		solveDiffClutch4();
	}

	void solveDiff4()
	{
		btScalar velocity1 = shaft[1]->ang_velocity + shaft[2]->ang_velocity;
		btScalar velocity2 = shaft[3]->ang_velocity + shaft[4]->ang_velocity;
		btScalar velocity = btScalar(0.25) * (velocity1 + velocity2);
		btScalar velocity_error = shaft[0]->ang_velocity - velocity * gear_ratio;
		btScalar impulse = clutch[0].solve(velocity_error);
		// not really physically correct torque splitting between front and rear
		btScalar impulse0 = btScalar(-0.5) * (impulse * gear_ratio);
		btScalar impulse1 = impulse0 * torque_split;
		btScalar impulse2 = impulse0 - impulse1;
		shaft[0]->applyImpulse(impulse);
		shaft[1]->applyImpulse(impulse1);
		shaft[2]->applyImpulse(impulse1);
		shaft[3]->applyImpulse(impulse2);
		shaft[4]->applyImpulse(impulse2);
	}

	void solveDiffClutch4()
	{
		btScalar load = Max(-clutch[0].impulse, clutch[0].impulse * clutch[1].decel_factor);
		if (load > 0)
		{
			btScalar v1 = shaft[1]->ang_velocity + shaft[2]->ang_velocity;
			btScalar v2 = shaft[3]->ang_velocity + shaft[4]->ang_velocity;
			btScalar velocity_error = btScalar(0.5) * (v1 - v2);
			btScalar impulse = clutch[1].solve(velocity_error, load);
			btScalar impulse4 = btScalar(0.25) * impulse;
			shaft[1]->applyImpulse(impulse4);
			shaft[2]->applyImpulse(impulse4);
			shaft[3]->applyImpulse(-impulse4);
			shaft[4]->applyImpulse(-impulse4);
		}

		load = Max(-clutch[0].impulse, clutch[0].impulse * clutch[2].decel_factor);
		if (load > 0)
			clutch[2].solve(*shaft[1], *shaft[2], load);

		load = Max(-clutch[0].impulse, clutch[0].impulse * clutch[3].decel_factor);
		if (load > 0)
			clutch[3].solve(*shaft[3], *shaft[4], load);
	}
};

#endif // _DRIVELINE_H
