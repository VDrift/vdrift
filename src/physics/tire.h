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

#ifndef _SIM_TIRE_H
#define _SIM_TIRE_H

#include "LinearMath/btVector3.h"
#include <vector>

namespace sim
{

struct TireInfo
{
	btScalar tread;						///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	btScalar max_load;					///< maximum tire load in kN
	btScalar max_camber;				///< maximum tire camber in degrees
	std::vector<btScalar> longitudinal;	///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector<btScalar> lateral;		///< the parameters of the lateral pacejka equation.  this is series a
	std::vector<btScalar> aligning;		///< the parameters of the aligning moment pacejka equation.  this is series c
	std::vector<btScalar> sigma_hat;	///< maximum grip in the longitudinal direction
	std::vector<btScalar> alpha_hat;	///< maximum grip in the lateral direction
	TireInfo();							///< default constructor
};

class Tire : private TireInfo
{
public:
	/// default constructor
	Tire();

	/// init tire
	void init(const TireInfo & info);

	/// get tire tread fraction
	btScalar getTread() const;

	/// normal_force: tire load in N
	/// friction_coeff: contact surface friction coefficient
	/// inclination: wheel inclination (camber) in degrees
	/// ang_velocity: tire angular velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface
	/// lat_velocty: tire lateral velocity relative to surface
	btVector3 getForce(
		btScalar normal_force,
		btScalar friction_coeff,
		btScalar inclination,
		btScalar ang_velocity,
		btScalar lon_velocty,
		btScalar lat_velocity);

	/// cached state, modified by getForce
	btScalar getFeedback() const;
	btScalar getSlide() const;
	btScalar getSlip() const;
	btScalar getIdealSlide() const;
	btScalar getIdealSlip() const;

	/// calculate tire squeal factor [0, 1] based on ideal slide/slip
	btScalar getSqueal() const;

	/// load is the normal force in N
	btScalar getMaxFx(btScalar load) const;

	/// load is the normal force in N, camber is in degrees
	btScalar getMaxFy(btScalar load, btScalar camber) const;

	/// load is the normal force in N, camber is in degrees
	btScalar getMaxMz(btScalar load, btScalar camber) const;

private:
	btScalar camber;			///< tire camber relative to track surface
	btScalar slide;				///< ratio of tire contact patch speed to road speed, minus one
	btScalar slip;				///< angle (in degrees) between the wheel heading and the wheel velocity
	btScalar ideal_slide;		///< ideal slide ratio
	btScalar ideal_slip;		///< ideal slip angle
	btScalar fx, fy, fz, mz;	///< contact force and aligning torque
	btScalar vx, vy;			///< contact velocity in tire space

	/// pacejka magic formula longitudinal friction, Fz in kN
	btScalar PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const;

	/// pacejka magic formula lateral friction, Fz in kN
	btScalar PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const;

	/// pacejka magic formula aligning torque, Fz in kN
	btScalar PacejkaMz(btScalar sigma, btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const;

	/// get ideal slide ratio, slip angle, load in kN
	void getSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const;

	/// find ideal slip, slide for given parameters, load in kN
	void findSigmaHatAlphaHat(btScalar load, btScalar & output_sigmahat, btScalar & output_alphahat, int iterations=400);

	/// calculate sigma_hat, alpha_hat tables
	void calculateSigmaHatAlphaHat(int tablesize=20);
};

// implementation

inline btScalar Tire::getTread() const
{
	return tread;
}

inline btScalar Tire::getFeedback() const
{
	return mz;
}

inline btScalar Tire::getSlide() const
{
	return slide;
}

inline btScalar Tire::getSlip() const
{
	return slip;
}

inline btScalar Tire::getIdealSlide() const
{
	return ideal_slide;
}

inline btScalar Tire::getIdealSlip() const
{
	return ideal_slip;
}

}

#endif
