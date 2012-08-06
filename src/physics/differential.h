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

#ifndef _SIM_DIFFERENTIAL_H
#define _SIM_DIFFERENTIAL_H

#include "shaft.h"

namespace sim
{

struct DifferentialInfo
{
	btScalar final_drive;			///< Gear ratio of the differential.
	btScalar anti_slip;				///< Maximum anti_slip torque.
	btScalar anti_slip_factor;		///< Anti_slip torque factor [0,1] for torque sensitive LSDs.
	btScalar deceleration_factor;	///< 0.0 for 1-way LSD, 1.0 for 2-way LSD, in between for 1.5-way LSD.
	btScalar torque_split;			///< Torque split factor, 0.0 applies all torque to side1, for epicyclic differentials.
	btScalar inertia;				///< Rotational inertia of differential + driving shaft
	DifferentialInfo();				///< Default constructor makes an S2000-like car.
};

class Differential : private DifferentialInfo
{
public:
	/// Default constructor (use init to initialize)
	Differential();
	
	/// Setup differential
	void init(const DifferentialInfo & info, Shaft & sha, Shaft & shb);
	
	/// Max differential friction coupling torque
	btScalar getAntiSlipTorque() const;
	
	/// Gear ratio between driving and driven shafts
	btScalar getFinalDrive() const;
	
	/// Driving shaft
	Shaft & getShaft();
	
	/// Driven shaft 1
	Shaft & getShaft1();
	
	/// Driven shaft 2
	Shaft & getShaft2();

private:
	Shaft shaft;
	Shaft * shaft_a;
	Shaft * shaft_b;
};

// implementation

inline DifferentialInfo::DifferentialInfo() :
	final_drive(4.1),
	anti_slip(600.0),
	anti_slip_factor(0),
	deceleration_factor(0),
	torque_split(0.5),
	inertia(0.25)
{
	// ctor
}

inline Differential::Differential()
{
	init(DifferentialInfo(), Shaft::getFixed(), Shaft::getFixed());
}

inline void Differential::init(const DifferentialInfo & info, Shaft & sha, Shaft & shb)
{
	DifferentialInfo::operator=(info);
	shaft.setInertia(info.inertia);
	shaft_a = &sha;
	shaft_b = &shb;
}

inline btScalar Differential::getAntiSlipTorque() const
{
	return anti_slip;
}

inline btScalar Differential::getFinalDrive() const
{
	return final_drive;
}

inline Shaft & Differential::getShaft()
{
	return shaft;
}

inline Shaft & Differential::getShaft1()
{
	return *shaft_a;
}

inline Shaft & Differential::getShaft2()
{
	return *shaft_b;
}

}

#endif
