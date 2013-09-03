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

#ifndef _CARTRANSMISSION_H
#define _CARTRANSMISSION_H

#include "LinearMath/btScalar.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CarTransmission
{
friend class joeserialize::Serializer;
public:
	//default constructor makes an S2000-like car
	CarTransmission() :
		forward_gears(0),
		reverse_gears(0),
		shift_time(0.2),
		gear(0),
		driveshaft_rpm(0),
		crankshaft_rpm(0)
	{
		gear_ratios[0] = 0.0;
	}

	int GetGear() const
	{
		return gear;
	}

	int GetForwardGears() const
	{
		return forward_gears;
	}

	int GetReverseGears() const
	{
		return reverse_gears;
	}

	void SetShiftTime(btScalar value)
	{
		shift_time = value;
	}

	btScalar GetShiftTime() const
	{
		return shift_time;
	}

	void Shift(int newgear)
	{
		if (newgear <= forward_gears && newgear >= -reverse_gears)
			gear = newgear;
	}

	///ratio is: driveshaft speed / crankshaft speed
	void SetGearRatio(int gear, btScalar ratio)
	{
		gear_ratios[gear] = ratio;

		//find out how many consecutive forward gears we have
		forward_gears = 0;
		int key = 1;
		while (gear_ratios.find (key) != gear_ratios.end ())
		{
			forward_gears++;
			key++;
		}

		//find out how many consecutive forward gears we have
		reverse_gears = 0;
		key = -1;
		while (gear_ratios.find (key) != gear_ratios.end ())
		{
			reverse_gears++;
			key--;
		}
	}

	btScalar GetGearRatio(int gear) const
	{
		btScalar ratio = 1.0;
		std::map<int, btScalar>::const_iterator i = gear_ratios.find(gear);
		if (i != gear_ratios.end()) ratio = i->second;
		return ratio;
	}

	btScalar GetCurrentGearRatio() const
	{
		return GetGearRatio(gear);
	}

	///get the torque on the driveshaft due to the given torque at the clutch
	btScalar GetTorque(btScalar clutch_torque)
	{
		return clutch_torque*gear_ratios[gear];
	}

	///get the rotational speed of the clutch given the rotational speed of the driveshaft
	btScalar CalculateClutchSpeed(btScalar driveshaft_speed)
	{
		driveshaft_rpm = driveshaft_speed * 30.0 / M_PI;
		crankshaft_rpm = driveshaft_speed * gear_ratios[gear] * 30.0 / M_PI;
		return driveshaft_speed * gear_ratios[gear];
	}

	///get the rotational speed of the clutch given the rotational speed of the driveshaft (const)
	btScalar GetClutchSpeed(btScalar driveshaft_speed) const
	{
		std::map<int, btScalar>::const_iterator i = gear_ratios.find(gear);
		assert(i != gear_ratios.end());
		return driveshaft_speed * i->second;
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Transmission---" << "\n";
		//out << "Gear ratio: " << gear_ratios.at(gear) << "\n";
		out << "Crankshaft RPM: " << crankshaft_rpm << "\n";
		out << "Driveshaft RPM: " << driveshaft_rpm << "\n";
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, gear);
		return true;
	}

private:
	//constants (not actually declared as const because they can be changed after object creation)
	std::map <int, btScalar> gear_ratios; ///< gear number and ratio.  reverse gears are negative integers. neutral is zero.
	int forward_gears; ///< the number of consecutive forward gears
	int reverse_gears; ///< the number of consecutive reverse gears
	btScalar shift_time; ///< transmission shift time

	//variables
	int gear; ///< the current gear

	//for info only
	btScalar driveshaft_rpm;
	btScalar crankshaft_rpm;
};

#endif
