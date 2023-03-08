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

#ifndef _CARTIRE1_H
#define _CARTIRE1_H

#include "LinearMath/btScalar.h"

struct CarTireState;
struct CarTireSlipLUT;

class CarTire1
{
public:
	/// normal_force: tire load in N
	/// rot_velocity: tire contact velocity (w * r) in m/s
	/// lon_velocty: tire longitudinal velocity relative to surface in m/s
	/// lat_velocty: tire lateral velocity relative to surface in m/s
	/// s: tire state (friction and camber should have valid values)
	void ComputeState(
		btScalar normal_force,
		btScalar rot_velocity,
		btScalar lon_velocity,
		btScalar lat_velocity,
		CarTireState & s) const;

	/// separate mz computation from ComputeState for performance
	void ComputeAligningTorque(
		btScalar normal_force,
		CarTireState & s) const;

	/// get tire tread fraction
	btScalar getTread() const { return tread; }

	/// rolling resistance magnitude
	btScalar getRollingResistance(btScalar velocity, btScalar resistance_factor) const;

	/// load is the normal force in newtons.
	btScalar getMaxFx(btScalar load) const;

	/// longitudinal force derivative at zero slip
	btScalar getMaxDx(btScalar load) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxFy(btScalar load, btScalar camber) const;

	/// lateral force derivative at zero slip angle
	btScalar getMaxDy(btScalar load, btScalar camber) const;

	/// horizontal and vertical fy function shift due to camber
	void getCamberShift(btScalar load, btScalar camber, btScalar & sh, btScalar & sv) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar getMaxMz(btScalar load, btScalar camber) const;

	/// init peak force slip lut
	void initSlipLUT(CarTireSlipLUT & t) const;

	CarTire1();

private:
	/// pacejka magic formula parameters for longitudinal force
	void PacejkaParamFx(btScalar Fz, btScalar p[6]) const;

	/// pacejka magic formula parameters for lateral force
	void PacejkaParamFy(btScalar Fz, btScalar gamma, btScalar p[6]) const;

	/// pacejka magic formula, for fx s is in percent, for fy in degrees
	btScalar Pacejka(const btScalar p[6], btScalar s) const;

	/// pacejka magic formula for longitudinal force
	btScalar PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff) const;

	/// pacejka magic formula for lateral force
	btScalar PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & camber_alpha) const;

	/// pacejka magic formula for aligning torque
	btScalar PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff) const;

	/// pacejka magic formula for the longitudinal combining factor
	btScalar PacejkaGx(btScalar sigma, btScalar alpha) const;

	/// pacejka magic formula for the lateral combining factor
	btScalar PacejkaGy(btScalar sigma, btScalar alpha) const;

	void findIdealSlip(btScalar load, btScalar output_slip[2], int iterations = 200) const;

public:
	btScalar longitudinal[11]; ///< the parameters of the longitudinal pacejka equation.  this is series b
	btScalar lateral[15]; ///< the parameters of the lateral pacejka equation.  this is series a
	btScalar aligning[18]; ///< the parameters of the aligning moment pacejka equation.  this is series c
	btScalar combining[4]; ///< force combining parameters
	btScalar rolling_resistance_quad; ///< quadratic rolling resistance on a hard surface
	btScalar rolling_resistance_lin; ///< linear rolling resistance on a hard surface
	btScalar tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
};

#endif
