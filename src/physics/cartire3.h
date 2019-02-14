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

#ifndef _CARTIRE3_H
#define _CARTIRE3_H

#include "LinearMath/btVector3.h"

struct CarTireInfo3
{
	btScalar radius; // Tire radius [m]
	btScalar width;	// Tire width [m]
	btScalar ar; // Tire aspect ratio
	btScalar pt; // Tire inflation pressure
	btScalar ktx; // Longitudinal tread stiffness [GPa/m]
	btScalar kty; // Lateral tread stiffness [GPa/m]
	btScalar kcb; // Carcass bending stiffness [N/mm]
	btScalar ccb; // Carcass bending due to camber coefficient
	btScalar cfy; // Lateral force correction factor
	btScalar dz0; // Displacement at which full width contact is achieved [m]
	btScalar p0; // Contact pressure at which friction coeff scaling is 1 [bar]
	btScalar mus; // Static friction coefficient
	btScalar muc; // Sliding friction coefficient
	btScalar vs; // Stribeck friction velocity [m/s]
	btScalar cr0; // Rolling resistance coefficient
	btScalar cr2; // Rolling resistance velocity^2 coefficient
	btScalar tread; // 1.0 pure off-road tire, 0.0 pure road tire

	CarTireInfo3();
};

class CarTire3 : private CarTireInfo3
{
public:
	CarTire3();

	void init(const CarTireInfo3 & info);

	/// normal_force: tire load in N
	/// friction_coeff: contact surface friction coefficient
	/// sin_camber: dot product of wheel axis and contact surface normal
	/// rot_velocity: tire contact velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface in m/s
	/// lat_velocty: tire lateral velocity relative to surface in m/s
	btVector3 getForce(
		btScalar normal_force,
		btScalar friction_coeff,
		btScalar sin_camber,
		btScalar rot_velocity,
		btScalar lon_velocity,
		btScalar lat_velocity);

	btScalar getRollingResistance(btScalar velocity, btScalar resistance_factor) const;

	btScalar getTread() const;

	btScalar getSlip() const;
	btScalar getSlipAngle() const;
	btScalar getIdealSlip() const;
	btScalar getIdealSlipAngle() const;
	btScalar getFx() const;
	btScalar getFy() const;
	btScalar getMz() const;

	/// load is the normal force in newtons.
	btScalar getMaxFx(btScalar load) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxFy(btScalar load, btScalar camber) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxMz(btScalar load, btScalar camber) const;

private:
	btScalar ideal_slip;
	btScalar ideal_slip_angle;
	btScalar slip;
	btScalar slip_angle;
	btScalar fx;
	btScalar fy;
	btScalar mz;

	// cached derived parameters
	btScalar rkz; // 1 / tire vertical stiffness
	btScalar rkb; // 1 / carcass bending stiffness
	btScalar rp0; // 1 / nominal contact pressure
	btScalar rvs; // 1 / stribeck velocity

	// ideal slip ratio and angle lookup tables
	static const unsigned slip_table_size = 20;
	btScalar ideal_slips[slip_table_size];
	btScalar ideal_slip_angles[slip_table_size];

	btVector3 getMaxForce(btScalar fz, bool needfx=true, bool needfy=true) const;

	void findIdealSlip(btScalar fz, btScalar & islip, btScalar & iangle) const;

	void getIdealSlip(btScalar fz, btScalar & islip, btScalar & iangle) const;

	void initSlipTables();
};


inline btScalar CarTire3::getRollingResistance(btScalar velocity, btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar rolling_resistance = cr0 * resistance_factor;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	rolling_resistance += cr2 * velocity * velocity;

	// rolling resistance direction
	btScalar resistance = (velocity < 0) ? rolling_resistance : -rolling_resistance;

	return resistance;
}

inline btScalar CarTire3::getTread() const
{
	return tread;
}

inline btScalar CarTire3::getSlip() const
{
	return slip;
}

inline btScalar CarTire3::getSlipAngle() const
{
	return slip_angle;
}

inline btScalar CarTire3::getIdealSlip() const
{
	return ideal_slip;
}

inline btScalar CarTire3::getIdealSlipAngle() const
{
	return ideal_slip_angle;
}

inline btScalar CarTire3::getFx() const
{
	return fx;
}

inline btScalar CarTire3::getFy() const
{
	return fy;
}

inline btScalar CarTire3::getMz() const
{
	return mz;
}

#endif // _CARTIRE3_H
