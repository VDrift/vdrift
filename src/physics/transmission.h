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

#ifndef _SIM_TRANSMISSION_H
#define _SIM_TRANSMISSION_H

#include "shaft.h"

namespace sim
{

struct TransmissionInfo
{
	std::vector<btScalar> gear_ratios;	///< reverse + 1 neutral + forward
	btScalar shift_time;				///< transmission shift time
	int forward_gears;					///< number of consecutive forward gears
	int reverse_gears;					///< number of consecutive reverse gears
	TransmissionInfo();					///< default constructor, no gears
};

class Transmission : private TransmissionInfo
{
public:
	/// default constructor (use init)
	Transmission();
	
	/// setup tranmission
	void init(const TransmissionInfo & info, Shaft & shaft);

	/// set gear in [-reverse_gears, forward_gears]
	void shift(int newgear);
	
	/// linked driven shaft
	Shaft & getShaft() const;
	
	/// current gear
	int getGear() const;

	/// number of forward gears
	int getForwardGears() const;

	/// number of reverse gears, usually one
	int getReverseGears() const;

	/// get gear ratio of gear
	btScalar getGearRatio(int gear) const;

	/// current gear ratio
	btScalar getGearRatio() const;

	/// transmission shift time
	btScalar getShiftTime() const;
	
	/// clutch side rpm
	btScalar getClutchRPM() const;

private:
	Shaft * drive_shaft;	///< linked  driven shaft
	int gear;				///< selected gear
};

// implementation

inline TransmissionInfo::TransmissionInfo() :
	gear_ratios(1, 0),
	shift_time(0),
	forward_gears(0),
	reverse_gears(0)
{
	// ctor
}

inline Transmission::Transmission() :
	drive_shaft(&Shaft::getFixed()),
	gear(0)
{
	// ctor
}

inline void Transmission::init(const TransmissionInfo & info, Shaft & shaft)
{
	TransmissionInfo::operator=(info);
	drive_shaft = &shaft;
}

inline void Transmission::shift(int newgear)
{
	if (newgear != gear &&
		newgear <= forward_gears &&
		newgear >= -reverse_gears)
	{
		gear = newgear;
	}
}

inline Shaft & Transmission::getShaft() const
{
	return *drive_shaft;
}

inline int Transmission::getGear() const
{
	return gear;
}

inline int Transmission::getForwardGears() const
{
	return forward_gears;
}

inline int Transmission::getReverseGears() const
{
	return reverse_gears;
}

inline btScalar Transmission::getGearRatio(int gear) const
{
	return gear_ratios[gear + reverse_gears];
}

inline btScalar Transmission::getGearRatio() const
{
	return gear_ratios[gear + reverse_gears];
}

inline btScalar Transmission::getShiftTime() const
{
	return shift_time;
}

inline btScalar Transmission::getClutchRPM() const
{
	return getGearRatio(gear) * drive_shaft->getAngularVelocity() * 30 / M_PI;
}

}

#endif
