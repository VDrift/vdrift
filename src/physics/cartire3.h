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

#include "LinearMath/btScalar.h"

struct CarTireState;

class CarTire3
{
public:
	/// normal_force: tire load in N
	/// friction_coeff: contact surface friction coefficient
	/// sin_camber: dot product of wheel axis and contact surface normal
	/// rot_velocity: tire contact velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface in m/s
	/// lat_velocty: tire lateral velocity relative to surface in m/s
	void ComputeState(
		btScalar normal_force,
		btScalar friction_coeff,
		btScalar sin_camber,
		btScalar rot_velocity,
		btScalar lon_velocity,
		btScalar lat_velocity,
		CarTireState & s) const;

	btScalar getTread() const { return tread; }

	/// rolling resistance magnitude
	btScalar getRollingResistance(btScalar velocity, btScalar resistance_factor) const;

	/// load is the normal force in newtons.
	btScalar getMaxFx(btScalar load) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxFy(btScalar load, btScalar camber) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxMz(btScalar load, btScalar camber) const;

	/// init LUTs
	void init();

	CarTire3();

private:
	void getMaxForce(
		btScalar fz,
		btScalar * pfx,
		btScalar * pfy,
		btScalar * pmz) const;

	void findIdealSlip(btScalar fz, btScalar & islip, btScalar & iangle) const;

	void getIdealSlip(btScalar fz, btScalar & islip, btScalar & iangle) const;

	void initSlipTables();

public:
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

	// cached derived parameters
	btScalar rkz; // 1 / tire vertical stiffness
	btScalar rkb; // 1 / carcass bending stiffness
	btScalar rp0; // 1 / nominal contact pressure
	btScalar rvs; // 1 / stribeck velocity

	// ideal slip ratio and angle lookup tables
	static const unsigned slip_table_size = 20;
	btScalar ideal_slips[slip_table_size];
	btScalar ideal_slip_angles[slip_table_size];
};

#endif // _CARTIRE3_H
