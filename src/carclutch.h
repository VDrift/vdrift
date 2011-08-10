#ifndef _CARCLUTCH_H
#define _CARCLUTCH_H

#include "LinearMath/btScalar.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CARCLUTCH
{
friend class joeserialize::Serializer;
private:
	//constants (not actually declared as const because they can be changed after object creation)
	btScalar sliding_friction; ///< torque on the clutch is found by dividing the clutch pressure by the value in the area tag and multiplying by the radius and sliding (friction) parameters
	btScalar radius; ///< torque on the clutch is found by dividing the clutch pressure by the value in the area tag and multiplying by the radius and sliding (friction) parameters
	btScalar area; ///< torque on the clutch is found by dividing the clutch pressure by the value in the area tag and multiplying by the radius and sliding (friction) parameters
	btScalar max_pressure; ///< maximum allowed pressure on the plates

	//variables
	btScalar clutch_position;
	bool locked;

	//for info only
	btScalar last_torque;
	btScalar engine_speed;
	btScalar drive_speed;


public:
	//default constructor makes an S2000-like car
	CARCLUTCH() :
		sliding_friction(0.27),
		radius(0.15),
		area(0.75),
		max_pressure(11079.26),
		clutch_position(0),
		locked(false),
		last_torque(0),
		engine_speed(0),
		drive_speed(0)
	{}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Clutch---" << "\n";
		out << "Clutch position: " << clutch_position << "\n";
		out << "Locked: " << locked << "\n";
		out << "Torque: " << last_torque << "\n";
		out << "Engine speed: " << engine_speed << "\n";
		out << "Drive speed: " << drive_speed << "\n";
	}

	void SetSlidingFriction ( const btScalar& value )
	{
		sliding_friction = value;
	}

	void SetRadius ( const btScalar& value )
	{
		radius = value;
	}

	void SetArea ( const btScalar& value )
	{
		area = value;
	}

	void SetMaxPressure ( const btScalar& value )
	{
		max_pressure = value;
	}

	///set the clutch engagement, where 1.0 is fully engaged
	void SetClutch ( const btScalar& value )
	{
		clutch_position = value;
	}

	btScalar GetClutch() const
	{
		return clutch_position;
	}

	btScalar GetTorque(btScalar n_engine_speed, btScalar n_drive_speed)
	{
		engine_speed = n_engine_speed;
		drive_speed = n_drive_speed;
		btScalar new_speed_diff = drive_speed - engine_speed;
		locked = true;

		btScalar torque_capacity = sliding_friction * max_pressure * area * radius; // constant
		btScalar max_torque = clutch_position * torque_capacity;
		btScalar friction_torque = max_torque * new_speed_diff;	// highly viscous coupling (locked clutch)
		if (friction_torque > max_torque)					// slipping clutch
		{
			friction_torque = max_torque;
			locked = false;
		}
		else if (friction_torque < -max_torque)
		{
			friction_torque = -max_torque;
			locked = false;
		}

		btScalar torque = friction_torque;
		last_torque = torque;
		return torque;
	}

	bool IsLocked() const
	{
		return locked;
	}

	btScalar GetLastTorque() const
	{
		return last_torque;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, clutch_position);
		_SERIALIZE_(s, locked);
		return true;
	}
};

#endif
