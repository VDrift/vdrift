#ifndef _CARDIFFERENTIAL_H
#define _CARDIFFERENTIAL_H

#include "LinearMath/btScalar.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

///a differential that supports speed-sensitive limited slip functionality.  epicyclic (torque splitting) operation is also provided.
class CARDIFFERENTIAL
{
friend class joeserialize::Serializer;
public:
	//default constructor makes an S2000-like car
	CARDIFFERENTIAL() :
		final_drive(4.1),
		anti_slip(600.0),
		anti_slip_torque(0),
		anti_slip_torque_deceleration_factor(0),
		torque_split(0.5),
		side1_speed(0),
		side2_speed(0),
		side1_torque(0),
		side2_torque(0)
	{
		// ctor
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Differential---" << "\n";
		out << "Side 1 RPM: " << side1_speed * 30.0 / 3.141593 << "\n";
		out << "Side 2 RPM: " << side2_speed * 30.0 / 3.141593 << "\n";
		out << "Side 1 Torque: " << side1_torque << "\n";
		out << "Side 2 Torque: " << side2_torque << "\n";
	}

	void SetFinalDrive ( const btScalar & value )
	{
		final_drive = value;
	}
	
	void SetAntiSlip(btScalar as, btScalar ast, btScalar astdf)
	{
		anti_slip = as;
		anti_slip_torque = ast;
		anti_slip_torque_deceleration_factor = astdf;
	}
	
	btScalar CalculateDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed)
	{
		side1_speed = new_side1_speed;
		side2_speed = new_side2_speed;
		return final_drive * (side1_speed + side2_speed) * 0.5;
	}
	
	btScalar GetDriveshaftSpeed(btScalar new_side1_speed, btScalar new_side2_speed) const
	{
		return final_drive * (side1_speed + side2_speed) * 0.5;
	}
	
	btScalar clamp(btScalar val, btScalar min, btScalar max) const
	{
		return std::max(std::min(val,max), min);
	}
	
	void ComputeWheelTorques(btScalar driveshaft_torque)
	{
		//determine torque from the anti-slip mechanism
		btScalar current_anti_slip = anti_slip;
		if (anti_slip_torque > 0) //if torque sensitive
			current_anti_slip = anti_slip_torque*driveshaft_torque; //TODO: add some minimum anti-slip
		if (current_anti_slip < 0) //determine behavior for deceleration
		{
			current_anti_slip *= -anti_slip_torque_deceleration_factor;
		}
		current_anti_slip = std::max(btScalar(0) ,current_anti_slip);
		btScalar drag = clamp(current_anti_slip * (side1_speed - side2_speed),-anti_slip,anti_slip);
		
		btScalar torque = driveshaft_torque * final_drive;
		side1_torque = torque * (1 - torque_split) - drag;
		side2_torque = torque * torque_split + drag;
	}

	const btScalar & GetSide1Torque() const
	{
		return side1_torque;
	}

	const btScalar & GetSide2Torque() const
	{
		return side2_torque;
	}
	
	const btScalar & GetSide1Speed() const
	{
		return side1_speed;
	}

	const btScalar & GetSide2Speed() const
	{
		return side2_speed;
	}

	btScalar GetFinalDrive() const
	{
		return final_drive;
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, side1_speed);
		_SERIALIZE_(s, side2_speed);
		_SERIALIZE_(s, side1_torque);
		_SERIALIZE_(s, side2_torque);
		return true;
	}

private:
	//constants (not actually declared as const because they can be changed after object creation)
	btScalar final_drive; ///< the gear ratio of the differential
	btScalar anti_slip; ///< this allows modelling of speed-sensitive limited-slip differentials.  this is the maximum anti_slip torque that will be applied and, for speed-sensitive limited-slip differentials, the anti-slip multiplier that's always applied.
	btScalar anti_slip_torque; ///< this allows modelling of torque sensitive limited-slip differentials.  this is the anti_slip dependence on torque.
	btScalar anti_slip_torque_deceleration_factor; ///< this allows modelling of torque sensitive limited-slip differentials that are 1.5 or 2-way.  set it to 0.0 for 1-way LSD, 1.0 for 2-way LSD, and somewhere in between for 1.5-way LSD.
	btScalar torque_split; ///< this allows modelling of epicyclic differentials.  this value ranges from 0.0 to 1.0 where 0.0 applies all torque to side1
	
	//variables
	///by convention, side1 is left or front, side2 is right or rear.
	btScalar side1_speed;
	btScalar side2_speed;
	btScalar side1_torque;
	btScalar side2_torque;
};

#endif
