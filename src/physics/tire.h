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

#ifndef _TIRE_H
#define _TIRE_H

#include "LinearMath/btVector3.h"

struct TireInfo
{
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
	static const int tablesize = 18; ///< sigma/alpha hat tablesize
	btScalar sigma_hat[tablesize];	///< maximum grip in the longitudinal direction
	btScalar alpha_hat[tablesize];	///< maximum grip in the lateral direction
	btScalar coefficients[CNUM];	///< pacejka tire coefficients
	btScalar nominal_load;			///< tire nominal load Fz0 in N
	btScalar max_load;				///< tire maximum load in N
	btScalar max_camber;			///< tire maximum camber in degrees
	btScalar roll_resistance_quad;	///< quadratic rolling resistance on a hard surface
	btScalar roll_resistance_lin;	///< linear rolling resistance on a hard surface
	btScalar tread;					///< 1.0 pure off-road tire, 0.0 pure road tire
	TireInfo();						///< default constructor
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

	/// normal_load: tire load in N
	/// friction_coeff: contact surface friction coefficient
	/// camber: wheel camber in rad, positive when tire top tilts to the right, viewed from rear
	/// rot_velocity: tire contact velocity (w * r)
	/// lon_velocty: tire longitudinal velocity relative to surface
	/// lat_velocty: tire lateral velocity relative to surface
	btVector3 getForce(
		btScalar normal_load,
		btScalar friction_coeff,
		btScalar camber,
		btScalar rot_velocity,
		btScalar lon_velocty,
		btScalar lat_velocity);

	btScalar getRollingResistance(
		const btScalar velocity,
		const btScalar resistance_factor) const;

	/// cached state, modified by getForce, slip_angle in rad
	btScalar getSlip() const;
	btScalar getSlipAngle() const;
	btScalar getIdealSlip() const;
	btScalar getIdealSlipAngle() const;
	btScalar getFx() const;
	btScalar getFy() const;
	btScalar getMz() const;

	/// calculate tire squeal factor [0, 1] based on ideal slide/slip
	btScalar getSqueal() const;

	/// load is the normal force in N
	btScalar getMaxFx(btScalar load) const;

	/// load is the normal force in N, camber is in rad
	btScalar getMaxFy(btScalar load, btScalar camber) const;

private:
	btScalar slip;				///< ratio of tire contact patch speed to road speed, minus one
	btScalar slip_angle;		///< angle (in degrees) between the wheel heading and the wheel velocity
	btScalar ideal_slip;		///< ideal slip ratio
	btScalar ideal_slip_angle;	///< ideal slip angle
	btScalar vx, vy;			///< contact velocity in tire space
	btScalar fx, fy, fz;		///< contact force in tire space
	btScalar mz;				///< aligning torque

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
		btScalar alpha);

	/// combined slip lateral factor
	btScalar PacejkaGy(
		btScalar sigma,
		btScalar alpha);

	/// combined slip lateral offset
	btScalar PacejkaSvy(
		btScalar sigma,
		btScalar alpha,
		btScalar gamma,
		btScalar dFz,
		btScalar Dy);

	/// get ideal slide ratio, slip angle
	void getSigmaHatAlphaHat(
		btScalar load,
		btScalar & sh,
		btScalar & ah) const;

	/// find ideal slip, slide for given parameters
	void findSigmaHatAlphaHat(
		btScalar load,
		btScalar & output_sigmahat,
		btScalar & output_alphahat,
		int iterations = 400);

	/// init sigma_hat, alpha_hat tables
	void initSigmaHatAlphaHat();
};

// implementation

inline btScalar Tire::getTread() const
{
	return tread;
}

inline btScalar Tire::getSlip() const
{
	return slip;
}

inline btScalar Tire::getSlipAngle() const
{
	return slip_angle;
}

inline btScalar Tire::getIdealSlip() const
{
	return ideal_slip;
}

inline btScalar Tire::getIdealSlipAngle() const
{
	return ideal_slip_angle;
}

inline btScalar Tire::getFx() const
{
	return fx;
}

inline btScalar Tire::getFy() const
{
	return fy;
}

inline btScalar Tire::getMz() const
{
	return mz;
}

inline btScalar Tire::getRollingResistance(
	const btScalar velocity,
	const btScalar resistance_factor) const
{
	// surface influence on rolling resistance
	btScalar rolling_resistance = resistance_factor * roll_resistance_lin;

	// heat due to tire deformation increases rolling resistance
	// approximate by quadratic function
	rolling_resistance += velocity * velocity * roll_resistance_quad;

	// rolling resistance direction
	btScalar resistance = -rolling_resistance;
	if (velocity < 0) resistance = -resistance;

	return resistance;
}

#endif

