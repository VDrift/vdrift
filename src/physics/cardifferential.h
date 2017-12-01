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

#ifndef _CARDIFFERENTIAL_H
#define _CARDIFFERENTIAL_H

#include "LinearMath/btScalar.h"
#include "macros.h"

/// A differential that supports speed-sensitive limited slip functionality.
/// Epicyclic (torque splitting) operation is also provided.

struct CarDifferentialInfo
{
	CarDifferentialInfo();

	btScalar final_drive; ///< The gear ratio of the differential.
	btScalar anti_slip; ///< This allows modelling of speed-sensitive limited-slip differentials. This is the maximum anti_slip torque that will be applied and, for speed-sensitive limited-slip differentials, the anti-slip multiplier that's always applied.
	btScalar anti_slip_torque; ///< This allows modelling of torque sensitive limited-slip differentials. This is the anti_slip dependence on torque.
	btScalar anti_slip_torque_deceleration_factor; ///< This allows modelling of torque sensitive limited-slip differentials that are 1.5 or 2-way. Set it to 0.0 for 1-way LSD, 1.0 for 2-way LSD, and somewhere in between for 1.5-way LSD.
	btScalar torque_split; ///< This allows modelling of epicyclic differentials. This value ranges from 0.0 to 1.0 where 0.0 applies all torque to side1.
};

class CarDifferential : private CarDifferentialInfo
{
public:
	/// Default constructor makes an S2000-like car.
	CarDifferential();

	void Init(const CarDifferentialInfo & info);

	btScalar CalculateDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed);

	btScalar GetDriveshaftSpeed() const;

	void ComputeWheelTorques(btScalar driveshaft_torque);

	btScalar GetSide1Torque() const;

	btScalar GetSide2Torque() const;

	btScalar GetSide1Speed() const;

	btScalar GetSide2Speed() const;

	btScalar GetFinalDrive() const;

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "---Differential---" << "\n";
		out << "Side 1 RPM: " << side1_speed * btScalar(30 / M_PI) << "\n";
		out << "Side 2 RPM: " << side2_speed * btScalar(30 / M_PI) << "\n";
		out << "Side 1 Torque: " << side1_torque << "\n";
		out << "Side 2 Torque: " << side2_torque << "\n";
	}

	template <class Serializer>
	bool Serialize(Serializer & s)
	{
		_SERIALIZE_(s, side1_speed);
		_SERIALIZE_(s, side2_speed);
		_SERIALIZE_(s, side1_torque);
		_SERIALIZE_(s, side2_torque);
		return true;
	}

private:
	/// By convention, side1 is left or front, side2 is right or rear.
	btScalar side1_speed;
	btScalar side2_speed;
	btScalar side1_torque;
	btScalar side2_torque;
};

#endif
