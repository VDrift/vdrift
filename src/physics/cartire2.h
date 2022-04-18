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

#ifndef _CARTIRE2_H
#define _CARTIRE2_H

#include "LinearMath/btScalar.h"

struct CarTireState;
struct CarTireSlipLUT;

class CarTire2
{
public:
	/// normal_load: tire load in N
	/// rot_velocity: tire contact velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface
	/// lat_velocty: tire lateral velocity relative to surface
	/// s: tire state (friction and camber should have valid values)
	void ComputeState(
		btScalar normal_load,
		btScalar rot_velocity,
		btScalar lon_velocity,
		btScalar lat_velocity,
		CarTireState & s) const;

	void ComputeAligningTorque(
		btScalar normal_load,
		CarTireState & s)
	{
		// Already computed in ComputeState
	}

	/// get tire tread fraction
	btScalar getTread() const { return tread; }

	/// rolling resistance magnitude
	btScalar getRollingResistance(btScalar velocity, btScalar resistance_factor) const;

	/// load is the normal force in N
	btScalar getMaxFx(btScalar load) const;

	/// load is the normal force in N, camber is in rad
	btScalar getMaxFy(btScalar load, btScalar camber) const;

	/// load is the normal force in N, camber is in rad
	btScalar getMaxMz(btScalar load, btScalar camber) const;

	/// init peak force slip lut
	void initSlipLUT(CarTireSlipLUT & t) const;

	CarTire2();

private:
	/// longitudinal friction
	btScalar PacejkaFx(
		btScalar sigma,
		btScalar Fz,
		btScalar dFz,
		btScalar friction_coeff) const;

	/// lateral friction
	btScalar PacejkaFy(
		btScalar alpha,
		btScalar gamma,
		btScalar Fz,
		btScalar dFz,
		btScalar friction_coeff,
		btScalar & Dy,
		btScalar & BCy,
		btScalar & Shf) const;

	/// aligning torque
	btScalar PacejkaMz(
		btScalar alpha,
		btScalar gamma,
		btScalar Fz,
		btScalar dFz,
		btScalar friction_coeff,
		btScalar Fy,
		btScalar BCy,
		btScalar Shf) const;

	/// combined slip longitudinal factor
	btScalar PacejkaGx(
		btScalar sigma,
		btScalar alpha) const;

	/// combined slip lateral factor
	btScalar PacejkaGy(
		btScalar sigma,
		btScalar alpha) const;

	/// combined slip lateral offset
	btScalar PacejkaSvy(
		btScalar sigma,
		btScalar alpha,
		btScalar gamma,
		btScalar dFz,
		btScalar Dy) const;

	/// get ideal slip, slip angle
	void getSigmaHatAlphaHat(
		btScalar load,
		btScalar & sh,
		btScalar & ah) const;

	/// find ideal slip, slip angle
	void findIdealSlip(
		btScalar load,
		btScalar output_slip[2],
		int iterations = 200) const;

// currently public for the loader
public:
	/// tire coefficients enumerator
	#define TIRE_COEFF_LIST \
	ENTRY(PCX1) \
	ENTRY(PDX1) ENTRY(PDX2) \
	ENTRY(PEX1) ENTRY(PEX2) ENTRY(PEX3) ENTRY(PEX4)\
	ENTRY(PKX1) ENTRY(PKX2) ENTRY(PKX3) \
	ENTRY(PHX1) ENTRY(PHX2) \
	ENTRY(PVX1) ENTRY(PVX2) \
	ENTRY(PCY1) \
	ENTRY(PDY1) ENTRY(PDY2) ENTRY(PDY3) \
	ENTRY(PEY1) ENTRY(PEY2) ENTRY(PEY3) ENTRY(PEY4) \
	ENTRY(PKY1) ENTRY(PKY2) ENTRY(PKY3) \
	ENTRY(PHY1) ENTRY(PHY2) ENTRY(PHY3) \
	ENTRY(PVY1) ENTRY(PVY2) ENTRY(PVY3) ENTRY(PVY4) \
	ENTRY(QBZ1) ENTRY(QBZ2) ENTRY(QBZ3) ENTRY(QBZ4) ENTRY(QBZ5) \
	ENTRY(QCZ1) \
	ENTRY(QDZ1) ENTRY(QDZ2) ENTRY(QDZ3) ENTRY(QDZ4) \
	ENTRY(QEZ1) ENTRY(QEZ2) ENTRY(QEZ3) ENTRY(QEZ4) ENTRY(QEZ5) \
	ENTRY(QHZ1) ENTRY(QHZ2) ENTRY(QHZ3) ENTRY(QHZ4) \
	ENTRY(QBZ9) ENTRY(QBZ10) \
	ENTRY(QDZ6) ENTRY(QDZ7) ENTRY(QDZ8) ENTRY(QDZ9) \
	ENTRY(RBX1) ENTRY(RBX2) \
	ENTRY(RCX1) \
	ENTRY(RHX1) \
	ENTRY(RBY1) ENTRY(RBY2) ENTRY(RBY3) \
	ENTRY(RCY1) \
	ENTRY(RHY1) \
	ENTRY(RVY1) ENTRY(RVY2) ENTRY(RVY3) ENTRY(RVY4) ENTRY(RVY5) ENTRY(RVY6)
	#define ENTRY(x) x,
	enum CoeffEnum
	{
		TIRE_COEFF_LIST
		CNUM
	};
	#undef ENTRY

	static const char * coeffname[]; ///< tire coefficients names
	btScalar coefficients[CNUM];	///< pacejka tire coefficients
	btScalar nominal_load;			///< tire nominal load Fz0 in N
	btScalar max_load;				///< tire maximum load in N
	btScalar max_camber;			///< tire maximum camber in degrees
	btScalar roll_resistance_quad;	///< quadratic rolling resistance on a hard surface
	btScalar roll_resistance_lin;	///< linear rolling resistance on a hard surface
	btScalar tread;					///< 1.0 pure off-road tire, 0.0 pure road tire
};

#endif
