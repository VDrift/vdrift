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

#include "engine.h"

namespace sim
{

EngineInfo::EngineInfo() :
	redline(7800),
	rpm_limit(9000),
	rpm_start(1000),
	rpm_stall(350),
	idle(0.02),
	fuel_rate(4E7),
	friction(0.000328),
	inertia(1),
	mass(200),
	fuel_mass(40),
	fuel_capacity(40),
	nos_mass(0),
	nos_boost(0),
	nos_fuel_ratio(5)
{
	// ctor
}

Engine::Engine()
{
	//init(EngineInfo());
}

void Engine::init(const EngineInfo & info)
{
	this->info = info;
	this->info.friction = calcEngineFriction();
	this->info.idle = calcIdleThrottle();
	shaft.setInertia(info.inertia);
	combustion_torque = 0;
	friction_torque = 0;
	throttle_position = 0;
	nos_boost_factor = 0;
	fuel_mass = info.fuel_mass;
	nos_mass = info.nos_mass;
	rev_limit_exceeded = false;
	stalled = true;
}

void Engine::start()
{
	btScalar dw = info.rpm_start * M_PI / 30 - shaft.getAngularVelocity();
	if (dw > 0)	shaft.applyImpulse(shaft.getInertia() * dw);
}

void Engine::update(btScalar dt)
{
	stalled = shaft.getAngularVelocity() <= info.rpm_stall * M_PI / 30;
	rev_limit_exceeded = shaft.getAngularVelocity() >= info.rpm_limit * M_PI / 30;

	if (fuel_mass < 1E-3 || rev_limit_exceeded || stalled)
	{
		combustion_torque = 0;

		friction_torque = getFrictionTorque(0, shaft.getAngularVelocity());

		if (stalled)
		{
			// try to model the static friction of the engine
			friction_torque *= 100.0;
		}
	}
	else
	{
		combustion_torque = getCombustionTorque(throttle_position, shaft.getAngularVelocity());

		friction_torque = getFrictionTorque(throttle_position, shaft.getAngularVelocity());

		btScalar fuel_consumed = 0;
		if (nos_mass > 0 && nos_boost_factor > 0)
		{
			// nitrous injection
			btScalar boost = nos_boost_factor * info.nos_boost;
			combustion_torque += boost / shaft.getAngularVelocity();

			fuel_consumed = boost * info.fuel_rate * dt;
			btScalar nos_consumed = info.nos_fuel_ratio * fuel_consumed;
			nos_mass = btMax(btScalar(0), nos_mass - nos_consumed);
		}
		btScalar power = combustion_torque * shaft.getAngularVelocity();
		fuel_consumed += info.fuel_rate * power * dt;

		fuel_mass = btMax(btScalar(0), fuel_mass - fuel_consumed);
	}
}

btScalar Engine::getCombustionTorque(btScalar throttle, btScalar angvel) const
{
	btScalar rpm = angvel * 30.0 / M_PI;
	if (rpm < info.rpm_stall || rpm > info.rpm_limit - 1E-3)
		return 0.0;

	btScalar scale = (info.torque.size() - 1) / (info.rpm_limit - info.rpm_stall); // constant
	btScalar f = (rpm - info.rpm_stall) * scale;
	int n = int(f); // floor(f)
	btScalar fraction = f - n;
	btAssert(n + 1 < int(info.torque.size()));
	btScalar torque = (1 - fraction) * info.torque[n] + fraction * info.torque[n + 1];
	return throttle * torque;
}

btScalar Engine::getFrictionTorque(btScalar throttle, btScalar angvel) const
{
	const btScalar scale = M_PI / 30.0;
	btClamp(angvel, info.rpm_stall * scale, info.rpm_limit * scale - 1);
	return getCombustionTorque(-0.25, angvel) * (1 - throttle);
/*
	btScalar velsign = angvel < 0 ? -1.0 : 1.0;
	btScalar A = 0;
	btScalar B = -600 * info.friction; //-1300 * info.friction;
	btScalar C = 0;
	return (A + angvel * B + -velsign * C * angvel * angvel) * (1.0 - throttle);
*/
}

btScalar Engine::calcIdleThrottle() const
{
	btScalar angvel_start = info.rpm_start * M_PI / 30.0;
	for (btScalar idle = 0; idle < 1.0; idle += 0.01)
	{
		if (getCombustionTorque(idle, angvel_start) > -getFrictionTorque(idle, angvel_start))
		{
			return idle;
		}
	}
	return 0;
}

btScalar Engine::calcEngineFriction() const
{
	btScalar max_power_angvel = info.redline * M_PI / 30.0;
	btScalar max_power = getCombustionTorque(1, max_power_angvel) * max_power_angvel;
	return max_power / (max_power_angvel * max_power_angvel * max_power_angvel);
}

}
