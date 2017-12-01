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

#include "cardifferential.h"
#include "minmax.h"

CarDifferentialInfo::CarDifferentialInfo() :
	final_drive(4.1),
	anti_slip(600),
	anti_slip_torque(0),
	anti_slip_torque_deceleration_factor(0),
	torque_split(0.5)
{
	// ctor
}

CarDifferential::CarDifferential() :
	side1_speed(0),
	side2_speed(0),
	side1_torque(0),
	side2_torque(0)
{
	// ctor
}

void CarDifferential::Init(const CarDifferentialInfo & info)
{
	*static_cast<CarDifferentialInfo*>(this) = info;
}

btScalar CarDifferential::CalculateDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed)
{
	side1_speed = new_side1_speed;
	side2_speed = new_side2_speed;
	return final_drive * (side1_speed + side2_speed) *  btScalar(0.5);
}

btScalar CarDifferential::GetDriveshaftSpeed() const
{
	return final_drive * (side1_speed + side2_speed) * btScalar(0.5);
}

void CarDifferential::ComputeWheelTorques(btScalar driveshaft_torque)
{
	// Determine torque from the anti-slip mechanism.
	btScalar current_anti_slip = anti_slip;
	// If torque sensitive.
	if (anti_slip_torque > 0)
		//TODO: add some minimum anti-slip.
		current_anti_slip = anti_slip_torque*driveshaft_torque;

	// Determine behavior for deceleration.
	if (current_anti_slip < 0)
		current_anti_slip *= -anti_slip_torque_deceleration_factor;

	current_anti_slip = Max(current_anti_slip, btScalar(0));
	btScalar drag = current_anti_slip * (side1_speed - side2_speed);
	drag = Clamp(drag, -anti_slip, anti_slip);

	btScalar torque = driveshaft_torque * final_drive;
	side1_torque = torque * (1 - torque_split) - drag;
	side2_torque = torque * torque_split + drag;
}

btScalar CarDifferential::GetSide1Torque() const
{
	return side1_torque;
}

btScalar CarDifferential::GetSide2Torque() const
{
	return side2_torque;
}

btScalar CarDifferential::GetSide1Speed() const
{
	return side1_speed;
}

btScalar CarDifferential::GetSide2Speed() const
{
	return side2_speed;
}

btScalar CarDifferential::GetFinalDrive() const
{
	return final_drive;
}
