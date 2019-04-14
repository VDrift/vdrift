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

#ifndef _CARBRAKE_H
#define _CARBRAKE_H

#include "LinearMath/btScalar.h"
#include "minmax.h"
#include "macros.h"

struct CarBrakeInfo
{
	btScalar friction; ///< sliding coefficient of friction for the brake pads on the rotor
	btScalar max_pressure; ///< maximum allowed pressure
	btScalar radius; ///< effective radius of the rotor
	btScalar area; ///< area of the brake pads
	btScalar brake_bias; ///< the fraction of the pressure to be applied to the brake
	btScalar handbrake_bias; ///< the fraction of the pressure to be applied when the handbrake is pulled

	CarBrakeInfo() :
		friction(0.73),
		max_pressure(4E6),
		radius(0.14),
		area(0.015),
		brake_bias(1),
		handbrake_bias(0)
	{ }
};

class CarBrake
{
public:
	// default constructor makes an S2000-like car
	CarBrake() :
		max_torque(6132),
		brake_bias(1),
		handbrake_bias(0),
		brake_factor(0),
		handbrake_factor(0),
		lasttorque(0)
	{
		// ctor
	}

	void Init(const CarBrakeInfo & info)
	{
		max_torque = (info.max_pressure * info.area) * (info.friction * info.radius);
		brake_bias = info.brake_bias;
		handbrake_bias = info.handbrake_bias;
	}

	/// brake_factor ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
	void SetBrakeFactor(btScalar newfactor)
	{
		brake_factor = newfactor;
	}

	/// ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
	void SetHandbrakeFactor(btScalar value)
	{
		handbrake_factor = value;
	}

	void SetBias(btScalar value)
	{
		brake_bias = value;
	}

	void SetHandbrake(btScalar value)
	{
		handbrake_bias = value;
	}

	btScalar GetBrakeFactor() const
	{
		return brake_factor;
	}

	/// brake torque magnitude
	btScalar GetTorque()
	{
		lasttorque = max_torque * (brake_bias * brake_factor + handbrake_bias * handbrake_factor);
		return lasttorque;
	}

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "---Brake---" << "\n";
		out << "Control: " << brake_factor << ", " << handbrake_factor << "\n";
		out << "Torque: " << lasttorque << "\n";
	}

	template <class Serializer>
	bool Serialize(Serializer & s)
	{
		_SERIALIZE_(s, brake_factor);
		_SERIALIZE_(s, handbrake_factor);
		return true;
	}

private:
	btScalar max_torque;
	btScalar brake_bias;
	btScalar handbrake_bias;
	btScalar brake_factor;
	btScalar handbrake_factor; ///< this is separate so that ABS does not get applied to the handbrake
	btScalar lasttorque; ///< for info only
};

#endif
