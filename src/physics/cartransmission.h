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
#include "macros.h"

#include <vector>

class CarTransmission
{
public:
	//default constructor makes an S2000-like car
	CarTransmission() :
		gear_ratios(1, 0),
		forward_gears(0),
		reverse_gears(0),
		shift_time(0.2),
		gear(0),
		driveshaft_rpm(0),
		crankshaft_rpm(0)
	{
		// ctor
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

	/// ratios vector  contains reverse + neutral(0) + forward gear ratios
	/// gear ratio is: driveshaft speed / crankshaft speed
	void SetGears(std::vector<btScalar> ratios, int forward, int reverse)
	{
		assert((int)ratios.size() == reverse + 1 + forward);
		gear_ratios = ratios;
		forward_gears = forward;
		reverse_gears = reverse;
	}

	btScalar GetGearRatio(int gear) const
	{
		assert(gear <= forward_gears && gear >= -reverse_gears);
		return gear_ratios[gear + reverse_gears];
	}

	btScalar GetCurrentGearRatio() const
	{
		return GetGearRatio(gear);
	}

	///get the rotational speed of the clutch given the rotational speed of the driveshaft
	btScalar CalculateClutchSpeed(btScalar driveshaft_speed)
	{
		auto crankshaft_speed = GetClutchSpeed(driveshaft_speed);
		driveshaft_rpm = driveshaft_speed * btScalar(30 / M_PI);
		crankshaft_rpm = crankshaft_speed * btScalar(30 / M_PI);
		return crankshaft_speed;
	}

	///get the rotational speed of the clutch given the rotational speed of the driveshaft (const)
	btScalar GetClutchSpeed(btScalar driveshaft_speed) const
	{
		return driveshaft_speed * GetCurrentGearRatio();
	}

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "---Transmission---" << "\n";
		//out << "Gear ratio: " << gear_ratios.at(gear) << "\n";
		out << "Crankshaft RPM: " << crankshaft_rpm << "\n";
		out << "Driveshaft RPM: " << driveshaft_rpm << "\n";
	}

	template <class Serializer>
	bool Serialize(Serializer & s)
	{
		_SERIALIZE_(s, gear);
		return true;
	}

private:
	//constants (not actually declared as const because they can be changed after object creation)
	std::vector<btScalar> gear_ratios; ///< reverse gears + neutral + forward gears
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
