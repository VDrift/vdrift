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

#ifndef _CARCLUTCH_H
#define _CARCLUTCH_H

#include "LinearMath/btScalar.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CarClutch
{
friend class joeserialize::Serializer;
private:
	btScalar max_torque;

	//variables
	btScalar clutch_position;
	bool locked;

	//for info only
	btScalar last_torque;
	btScalar engine_speed;
	btScalar drive_speed;


public:
	/// default constructor makes an S2000-like car
	CarClutch() :
		max_torque(0),
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

	void Set(btScalar sliding_friction, btScalar max_pressure, btScalar area, btScalar radius)
	{
		max_torque = sliding_friction * max_pressure * area * radius;
	}

	void SetPosition(btScalar value)
	{
		clutch_position = value;
	}

	btScalar GetPosition() const
	{
		return clutch_position;
	}

	btScalar GetMaxTorque() const
	{
		return max_torque;
	}

	btScalar GetTorque(btScalar n_engine_speed, btScalar n_drive_speed)
	{
		engine_speed = n_engine_speed;
		drive_speed = n_drive_speed;
		locked = true;

		btScalar new_speed_diff = drive_speed - engine_speed;
		btScalar torque_limit = clutch_position * max_torque;
		btScalar friction_torque = torque_limit * new_speed_diff;	// highly viscous coupling (locked clutch)
		if (friction_torque > torque_limit)							// slipping clutch
		{
			friction_torque = torque_limit;
			locked = false;
		}
		else if (friction_torque < -torque_limit)
		{
			friction_torque = -torque_limit;
			locked = false;
		}

		last_torque = friction_torque;
		return friction_torque;
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
