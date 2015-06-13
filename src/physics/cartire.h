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

#ifndef _CARTIRE_H
#define _CARTIRE_H

#ifdef VDRIFTN

#include "physics/tire.h"
typedef Tire CarTire;
typedef TireInfo CarTireInfo;

#else

#include "LinearMath/btVector3.h"
#include "joeserialize.h"
#include "macros.h"

#include <vector>

struct CarTireInfo
{
	std::vector<btScalar> longitudinal; ///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector<btScalar> lateral; ///< the parameters of the lateral pacejka equation.  this is series a
	std::vector<btScalar> aligning; ///< the parameters of the aligning moment pacejka equation.  this is series c
	std::vector<btScalar> sigma_hat; ///< maximum grip in the longitudinal direction
	std::vector<btScalar> alpha_hat; ///< maximum grip in the lateral direction
	btScalar rolling_resistance_quad; ///< quadratic rolling resistance on a hard surface
	btScalar rolling_resistance_lin; ///< linear rolling resistance on a hard surface
	btScalar tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	CarTireInfo();
};

class CarTire : private CarTireInfo
{
friend class joeserialize::Serializer;
public:
	CarTire();

	void init(const CarTireInfo & info);

	/// get tire tread fraction
	btScalar getTread() const;

	/// normal_force: tire load in N
	/// friction_coeff: contact surface friction coefficient
	/// inclination: wheel inclination in degrees
	/// rot_velocity: tire contact velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface in m/s
	/// lat_velocty: tire lateral velocity relative to surface in m/s
	btVector3 getForce(
		btScalar normal_force,
		btScalar friction_coeff,
		btScalar inclination,
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

	bool Serialize(joeserialize::Serializer & s);

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

	void getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const;

	void findSigmaHatAlphaHat(
		btScalar load,
		btScalar & output_sigmahat,
		btScalar & output_alphahat,
		int iterations=400);

	void initSigmaHatAlphaHat(int tablesize = 20);
};

// implementation

inline btScalar CarTire::getTread() const
{
	return tread;
}

inline btScalar CarTire::getSlip() const
{
	return slide;
}

inline btScalar CarTire::getSlipAngle() const
{
	return slip;
}

inline btScalar CarTire::getIdealSlip() const
{
	return ideal_slide;
}

inline btScalar CarTire::getIdealSlipAngle() const
{
	return ideal_slip;
}

inline btScalar CarTire::getFx() const
{
	return fx;
}

inline btScalar CarTire::getFy() const
{
	return fy;
}

inline btScalar CarTire::getMz() const
{
	return mz;
}

inline bool CarTire::Serialize(joeserialize::Serializer & /*s*/)
{
	//_SERIALIZE_(s, mz); FIXME?
	return true;
}

#endif

#endif
