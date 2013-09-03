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

#ifndef _CARWHEEL_H
#define _CARWHEEL_H

#include "driveshaft.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CarWheel
{
friend class joeserialize::Serializer;
public:
	CarWheel() : radius(0.3), width(0.2), mass(20) {}

	btScalar GetRotation() const
	{
		return shaft.angle;
	}

	btScalar GetRPM() const
	{
		return shaft.ang_velocity * 30.0 / 3.141593;
	}

	btScalar GetAngularVelocity() const
	{
		return shaft.ang_velocity;
	}

	void SetAngularVelocity(btScalar value)
	{
		shaft.ang_velocity = value;
	}

	void SetMass(btScalar value)
	{
		mass = value;
	}

	btScalar GetMass() const
	{
		return mass;
	}

	void SetWidth(float value)
	{
		width = value;
	}

	btScalar GetWidth() const
	{
		return width;
	}

	void SetRadius(float value)
	{
		radius = value;
	}

	btScalar GetRadius() const
	{
		return radius;
	}

	void SetInertia(btScalar value)
	{
		shaft.inv_inertia = 1 / value;
	}

	btScalar GetInertia() const
	{
		return 1 / shaft.inv_inertia;
	}

	void Integrate(btScalar dt)
	{
		shaft.integrate(dt);
	}

	btScalar GetTorque(btScalar new_angvel, btScalar dt) const
	{
		return shaft.getMomentum(new_angvel) / dt;
	}

	void SetTorque(btScalar torque, btScalar dt)
	{
		shaft.applyMomentum(torque * dt);
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Wheel---" << "\n";
		out << "RPM: " << GetRPM() << "\n";
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, shaft.ang_velocity);
		_SERIALIZE_(s, shaft.angle);
		return true;
	}

private:
	DriveShaft shaft;
	btScalar radius;
	btScalar width;
	btScalar mass;
};

#endif
