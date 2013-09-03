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

CarDifferential::CarDifferential() : final_drive(4.1), anti_slip(600.0), anti_slip_torque(0), anti_slip_torque_deceleration_factor(0), torque_split(0.5), side1_speed(0), side2_speed(0), side1_torque(0), side2_torque(0)
{
	// Constructor.
}

void CarDifferential::DebugPrint(std::ostream & out) const
{
	out << "---Differential---" << "\n";
	out << "Side 1 RPM: " << side1_speed * 30.0 / 3.141593 << "\n";
	out << "Side 2 RPM: " << side2_speed * 30.0 / 3.141593 << "\n";
	out << "Side 1 Torque: " << side1_torque << "\n";
	out << "Side 2 Torque: " << side2_torque << "\n";
}

void CarDifferential::SetFinalDrive(const btScalar & value)
{
	final_drive = value;
}

void CarDifferential::SetAntiSlip(btScalar as, btScalar ast, btScalar astdf)
{
	anti_slip = as;
	anti_slip_torque = ast;
	anti_slip_torque_deceleration_factor = astdf;
}

btScalar CarDifferential::CalculateDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed)
{
	side1_speed = new_side1_speed;
	side2_speed = new_side2_speed;
	return final_drive * (side1_speed + side2_speed) * 0.5;
}

btScalar CarDifferential::GetDriveshaftSpeed() const
{
	return final_drive * (side1_speed + side2_speed) * 0.5;
}

btScalar CarDifferential::clamp(btScalar val, btScalar min, btScalar max) const
{
	return std::max(std::min(val,max), min);
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

	current_anti_slip = std::max(btScalar(0) ,current_anti_slip);
	btScalar drag = clamp(current_anti_slip * (side1_speed - side2_speed),-anti_slip,anti_slip);

	btScalar torque = driveshaft_torque * final_drive;
	side1_torque = torque * (1 - torque_split) - drag;
	side2_torque = torque * torque_split + drag;
}

const btScalar & CarDifferential::GetSide1Torque() const
{
	return side1_torque;
}

const btScalar & CarDifferential::GetSide2Torque() const
{
	return side2_torque;
}

const btScalar & CarDifferential::GetSide1Speed() const
{
	return side1_speed;
}

const btScalar & CarDifferential::GetSide2Speed() const
{
	return side2_speed;
}

btScalar CarDifferential::GetFinalDrive() const
{
	return final_drive;
}

bool CarDifferential::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, side1_speed);
	_SERIALIZE_(s, side2_speed);
	_SERIALIZE_(s, side1_torque);
	_SERIALIZE_(s, side2_torque);
	return true;
}
