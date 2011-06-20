#ifndef _CARWHEEL_H
#define _CARWHEEL_H

#include "driveshaft.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CARWHEEL
{
friend class joeserialize::Serializer;
public:
	CARWHEEL() : mass(20) {}

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
	btScalar mass;
};

#endif
