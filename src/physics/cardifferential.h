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

#include "joeserialize.h"
#include "LinearMath/btScalar.h"
#include "macros.h"

/// A differential that supports speed-sensitive limited slip functionality. Epicyclic (torque splitting) operation is also provided.
class CarDifferential
{
friend class joeserialize::Serializer;
public:
	/// Default constructor makes an S2000-like car.
	CarDifferential();

	void DebugPrint(std::ostream & out) const;

	void SetFinalDrive(const btScalar & value);

	void SetAntiSlip(btScalar as, btScalar ast, btScalar astdf);

	btScalar CalculateDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed);

	btScalar GetDriveshaftSpeed() const;

	btScalar clamp(btScalar val, btScalar min, btScalar max) const;

	void ComputeWheelTorques(btScalar driveshaft_torque);

	const btScalar & GetSide1Torque() const;

	const btScalar & GetSide2Torque() const;

	const btScalar & GetSide1Speed() const;

	const btScalar & GetSide2Speed() const;

	btScalar GetFinalDrive() const;

	bool Serialize(joeserialize::Serializer & s);

private:
	// Constants (not actually declared as const because they can be changed after object creation).
	btScalar final_drive; ///< The gear ratio of the differential.
	btScalar anti_slip; ///< This allows modelling of speed-sensitive limited-slip differentials. This is the maximum anti_slip torque that will be applied and, for speed-sensitive limited-slip differentials, the anti-slip multiplier that's always applied.
	btScalar anti_slip_torque; ///< This allows modelling of torque sensitive limited-slip differentials. This is the anti_slip dependence on torque.
	btScalar anti_slip_torque_deceleration_factor; ///< This allows modelling of torque sensitive limited-slip differentials that are 1.5 or 2-way. Set it to 0.0 for 1-way LSD, 1.0 for 2-way LSD, and somewhere in between for 1.5-way LSD.
	btScalar torque_split; ///< This allows modelling of epicyclic differentials. This value ranges from 0.0 to 1.0 where 0.0 applies all torque to side1.

	// Variables.
	/// By convention, side1 is left or front, side2 is right or rear.
	btScalar side1_speed;
	btScalar side2_speed;
	btScalar side1_torque;
	btScalar side2_torque;
};

#endif
