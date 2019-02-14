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

#include "LinearMath/btVector3.h"
#include "macros.h"

#include <vector>

struct CarTireInfo1
{
	std::vector<btScalar> longitudinal; ///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector<btScalar> lateral; ///< the parameters of the lateral pacejka equation.  this is series a
	std::vector<btScalar> aligning; ///< the parameters of the aligning moment pacejka equation.  this is series c
	std::vector<btScalar> combining; ///< force combining parameters
	std::vector<btScalar> sigma_hat; ///< maximum grip in the longitudinal direction
	std::vector<btScalar> alpha_hat; ///< maximum grip in the lateral direction
	btScalar rolling_resistance_quad; ///< quadratic rolling resistance on a hard surface
	btScalar rolling_resistance_lin; ///< linear rolling resistance on a hard surface
	btScalar tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	CarTireInfo1();
};

class CarTire1 : private CarTireInfo1
{
public:
	CarTire1();

	void init(const CarTireInfo1 & info);

	/// get tire tread fraction
	btScalar getTread() const;

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
		btScalar lon_velocty,
		btScalar lat_velocity);

	btScalar getRollingResistance(
		const btScalar velocity,
		const btScalar resistance_factor) const;

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

	template <class Serializer>
	bool Serialize(Serializer & s);

private:
	btScalar camber; ///< tire camber relative to track surface
	btScalar slide; ///< ratio of tire contact patch speed to road speed, minus one
	btScalar slip; ///< the angle (in degrees) between the wheel heading and the wheel's actual velocity
	btScalar ideal_slide; ///< ideal slide ratio
	btScalar ideal_slip; ///< ideal slip angle
	btScalar fx, fy, fz, mz;

	/// pacejka magic formula function, longitudinal
	btScalar PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const;

	/// pacejka magic formula function, lateral
	btScalar PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const;

	/// pacejka magic formula function, aligning
	btScalar PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const;

	/// pacejka magic formula longitudinal combining factor
	btScalar PacejkaGx(btScalar sigma, btScalar alpha);

	/// pacejka magic formula lateral combining factor
	btScalar PacejkaGy(btScalar sigma, btScalar alpha);

	void getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const;

	void findSigmaHatAlphaHat(
		btScalar load,
		btScalar & output_sigmahat,
		btScalar & output_alphahat,
		int iterations = 200);

	void initSigmaHatAlphaHat(int tablesize = 20);
};

// implementation

inline btScalar CarTire1::getTread() const
{
	return tread;
}

inline btScalar CarTire1::getSlip() const
{
	return slide;
}

inline btScalar CarTire1::getSlipAngle() const
{
	return slip;
}

inline btScalar CarTire1::getIdealSlip() const
{
	return ideal_slide;
}

inline btScalar CarTire1::getIdealSlipAngle() const
{
	return ideal_slip;
}

inline btScalar CarTire1::getFx() const
{
	return fx;
}

inline btScalar CarTire1::getFy() const
{
	return fy;
}

inline btScalar CarTire1::getMz() const
{
	return mz;
}

template <class Serializer>
inline bool CarTire1::Serialize(Serializer & /*s*/)
{
	return true;
}

#endif
