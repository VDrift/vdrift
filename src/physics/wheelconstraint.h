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
#include "cartire.h"
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

#include <fstream>

struct WheelConstraint
{
	btRigidBody * body;
	DriveShaft * shaft;
	ConstraintRow constraint[3];
	btVector3 position;
	btScalar radius;
	btScalar vcam;

	static std::ofstream dlog;
	
	void init(btScalar softness, btScalar error)
	{
		constraint[0].initMassInertia(*body, position);
		constraint[0].mass = constraint[0].mass * shaft->inertia /
			(constraint[0].mass * radius * radius + shaft->inertia);
		constraint[0].impulse = 0;

		constraint[1].initMassInertia(*body, position);
		constraint[1].impulse = 0;

		constraint[2].initMassInertia(*body, position, softness);
		constraint[2].rhs = -error * constraint[2].mass;
		constraint[2].cfm = -softness * constraint[2].mass;
		constraint[2].lower_impulse_limit = 0;
		constraint[2].upper_impulse_limit = error < 0 ? SIMD_INFINITY : 0;
		constraint[2].impulse = 0;
	}

	void init(btScalar stiffness, btScalar damping, btScalar displacement, btScalar dt)
	{
		btScalar softness = 1 / (dt * (dt * stiffness + damping));
		btScalar bias = stiffness / (dt * stiffness + damping);
		btScalar error = -bias * displacement;
		init(softness, error);
	}

	void getContactVelocity(btScalar v[3]) const
	{
		btVector3 vb = body->getVelocityInLocalPoint(position);
		v[0] = constraint[0].axis.dot(vb);
		v[1] = constraint[1].axis.dot(vb);
		v[2] = shaft->ang_velocity * radius;
	}

	void solveFriction()
	{
		btScalar ve[3];
		getContactVelocity(ve);
		ve[0] -= ve[2];
		ve[1] -= vcam;

		btScalar dp[2];
		btVector3 w[2];
		btVector3 p[2];
		for (int i = 0; i < 2; i++)
		{
			dp[i] = constraint[i].solveHard(ve[i]);
			w[i] = constraint[i].inv_inertia * dp[i];
			p[i] = constraint[i].axis * dp[i];
		}
		btVector3 dw = w[0] + w[1];
		btVector3 dv = (p[0] + p[1]) * body->getInvMass();

		body->setAngularVelocity(body->getAngularVelocity() + dw);
		body->setLinearVelocity(body->getLinearVelocity() + dv);
		shaft->applyImpulse(-dp[0] * radius);
	}

	void solveFriction(btScalar px[8], btScalar py[8], btScalar rdt)
	{
		btScalar v[3];
		getContactVelocity(v);
		btScalar vxe = v[0] - v[2];
		btScalar vye = v[1];
		btScalar ravbx = 1 / Max(std::abs(v[0]), btScalar(1E-9));

		auto & cx = constraint[0];
		auto & cy = constraint[1];
		btScalar mx = cx.mass * rdt;
		btScalar my = cy.mass * rdt;
		btScalar vxo = vxe - cx.impulse;
		btScalar vyo = vye - cy.impulse;
		//dlog << vxo << " " << vyo << " " << vxe << " " << vye << "\n";

		// initial slip velocity estimate
		btScalar fxmax = px[2];
		btScalar fymax = py[2];
		btScalar dfx0 = (px[0] * px[1]) * (100 * fxmax) * ravbx;
		btScalar dfy0 = (py[0] * py[1]) * RadToDeg(fymax) * ravbx;
		btScalar vxa = (mx * vxo) / (mx + dfx0);
		btScalar vya = (my * vyo) / (my + dfy0);
		btScalar vxb = vxa;
		btScalar vyb = vya;
		if (std::abs(dfx0 * vxb) > fxmax) vxb = vxo - std::copysign(fxmax, vxo) / mx;
		if (std::abs(dfy0 * vyb) > fymax) vyb = vyo - std::copysign(fymax, vyo) / my;
		btScalar vx = vxb;
		btScalar vy = vyb;

		// iterative slip velocity solver
		btScalar s, a;
		btScalar fx, fy;
		btScalar gx, gy;
		btScalar dvx = -vx;
		btScalar dvy = -vy;
		for (int n = 0; n < 3; ++n)
		{
			btScalar sn = vx * ravbx;
			btScalar an = Atan(vy * ravbx);
			s = sn * 100;
			a = RadToDeg(an);

			btScalar dfx, dgxx;
			btScalar dfy, dgyy;
			fx = PacejkaF(px, s, dfx);
			gx = PacejkaG(&px[6], sn, an, dgxx);
			fy = PacejkaF(py, a, dfy);
			gy = PacejkaG(&py[6], an, sn, dgyy);

			btScalar ds = ravbx; // d/dvx vx * ravbx
			btScalar da = ravbx / ((ravbx * ravbx) * (vy * vy) + 1); // d/dvy atan(vy * ravbx)
			btScalar dfgx = (fx * dgxx + (100 * dfx) * gx) * ds;
			btScalar dfgy = (fy * dgyy + RadToDeg(dfy) * gy) * da;
			btScalar fgx = fx * gx;
			btScalar fgy = fy * gy;

			btScalar dx = mx + dfgx;
			btScalar dy = my + dfgy;
			btScalar vxn = (dx > 0) ? (mx * vxo - fgx + dfgx * vx) / dx : vxo - fgx / mx;
			btScalar vyn = (dy > 0) ? (my * vyo - fgy + dfgy * vy) / dy : vyo - fgy / my;
			/*if (vx * vxn < 0)
			{
				dlog << n << " "
					<< vxa << " " << vxb << " "
					<< vx << " " << vxn << " "
					<< dfgx << " * x + " << fx * gx - dfgx * vx << ", "
					<< mx << " * (" << vxo << " - x), "
					<< gx << " * " << px[2] << " * sin(" << px[1] << " * atan("
					<< px[0] * (100 * ravbx) << "* x - " << px[3] << " * ("
					<< px[0] * (100 * ravbx) << "* x - atan("
					<< px[0] * (100 * ravbx) << "* x))))\n";
			}*/
			// when overshoot limit backstep to 0.618 of previous step
			btScalar dvxn = vxn - vx;
			btScalar dvyn = vyn - vy;
			btScalar dvxh = -0.618f * dvx;
			btScalar dvyh = -0.618f * dvy;
			if (dvx * dvxn < dvx * dvxh) {
				//dlog << n << " x " << dvx << " " << dvxn << " " << vx << " " << vxn << "\n";
				dvxn = dvxh;
			}
			if (dvy * dvyn < dvy * dvyh) {
				//dlog << n << " y " << dvy << " " << dvyn << " " << vy << " " << vyn << "\n";
				dvyn = dvyh;
			}
			dvx = dvxn;
			dvy = dvyn;
			vx += dvx;
			vy += dvy;
			//dlog << n << " " << vx << " " << vy << " " << fx << " " << fy << "\n";
		}

		btScalar dvxn = vx - vxo;
		btScalar dvyn = vy - vyo;
		dvx = dvxn - cx.impulse;
		dvy = dvyn - cy.impulse;
		//dlog << dvxn << " " << dvyn << " " << dvx << " " << dvy << "\n";

		cx.impulse = dvxn;
		cy.impulse = dvyn;
		btScalar dpx = cx.mass * dvx;
		btScalar dpy = cy.mass * dvy;
		btVector3 dw = cx.inv_inertia * dpx + cy.inv_inertia * dpy;
		btVector3 dv = (cx.axis * dpx + cy.axis * dpy) * body->getInvMass();

		body->setAngularVelocity(body->getAngularVelocity() + dw);
		body->setLinearVelocity(body->getLinearVelocity() + dv);
		shaft->applyImpulse(-dpx * radius);

		//getContactVelocity(v);
		//vxe = v[0] - v[2];
		//vye = v[1];
		//dlog << vxe << " " << vye << " " << cx.impulse << " " << cy.impulse << "\n";
	}

	void solveSuspension()
	{
		btVector3 vb = body->getVelocityInLocalPoint(position);
		btScalar ve = constraint[2].axis.dot(vb);

		btScalar dp = constraint[2].solve(ve);
		btVector3 dw = constraint[2].inv_inertia * dp;
		btVector3 dv = constraint[2].axis * (dp * body->getInvMass());

		body->setAngularVelocity(body->getAngularVelocity() + dw);
		body->setLinearVelocity(body->getLinearVelocity() + dv);
	}
};

#endif // _WHEEL_CONSTRAINT_H

