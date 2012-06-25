#ifndef _CARTIRE_H
#define _CARTIRE_H

#include "LinearMath/btVector3.h"
#include "joeserialize.h"
#include "macros.h"

#include <vector>
#include <string>
#include <iostream>

class PTree;

class CARTIRE
{
friend class joeserialize::Serializer;
public:
	CARTIRE();

	btScalar GetRadius() const	{return radius;}
	btScalar GetTread() const	{return tread;}
	btScalar GetAspectRatio() const	{return aspect_ratio;}
	btScalar GetWidth() const	{return sidewall_width;}

	btScalar GetFeedback() const	{return feedback;}
	btScalar GetSlide() const	{return slide;}
	btScalar GetSlip() const	{return slip;}
	btScalar GetIdealSlide() const	{return ideal_slide;}
	btScalar GetIdealSlip() const	{return ideal_slip;}

	/// load tire from config file
	bool Load(const PTree & cfg, std::ostream & error);

	/// look up ideal slide ratio, slip angle
	void GetSigmaHatAlphaHat(btScalar load, btScalar & sh, btScalar & ah) const;

	/// velocity vector of the wheel's reference frame in m/s
	/// velocity_ang: wheel angular veloity in rad/s
	/// inclination: wheel inclination in degrees
	/// normal_force: tire load in newton
	btVector3 GetForce(
		btScalar normal_force,
		btScalar friction_coeff,
		btScalar inclination,
		btScalar ang_velocity,
		btScalar lon_velocty,
		btScalar lat_velocity);

	/// get rolling resistance
	btScalar GetRollingResistance(const btScalar velocity, const btScalar rolling_resistance_factor) const;

	/// load is the normal force in newtons.
	btScalar GetMaxFx(btScalar load) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar GetMaxFy(btScalar load, btScalar camber) const;

	/// load is the normal force in newtons, camber is in degrees
	btScalar GetMaxMz(btScalar load, btScalar camber) const;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, feedback);
		return true;
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Tire---" << "\n";
		out << "Camber: " << camber << "\n";
		out << "Slide ratio: " << slide  << "\n";
		out << "Slip angle: " << slip  << "\n";
		//out << "Fx: " << fx << "\n";
		//out << "Fy: " << fy  << "\n";
		//out << "Fz: " << fz  << "\n";
	}

private:
	// constants
	btScalar radius; ///< the total tread radius of the tire in meters
	btScalar aspect_ratio; ///< 0..1 percentage value
	btScalar sidewall_width; /// in meters
	btScalar tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	btScalar rolling_resistance_linear; ///< linear rolling resistance on a hard surface
	btScalar rolling_resistance_quadratic; ///< quadratic rolling resistance on a hard surface
	std::vector<btScalar> longitudinal; ///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector<btScalar> lateral; ///< the parameters of the lateral pacejka equation.  this is series a
	std::vector<btScalar> aligning; ///< the parameters of the aligning moment pacejka equation.  this is series c
	std::vector<btScalar> sigma_hat; ///< maximum grip in the longitudinal direction
	std::vector<btScalar> alpha_hat; ///< maximum grip in the lateral direction

	// variables
	btScalar feedback; ///< the force feedback effect value
	btScalar camber; ///< tire camber relative to track surface
	btScalar slide; ///< ratio of tire contact patch speed to road speed, minus one
	btScalar slip; ///< the angle (in degrees) between the wheel heading and the wheel's actual velocity
	btScalar ideal_slide; ///< ideal slide ratio
	btScalar ideal_slip; ///< ideal slip angle

	// debugging
	btScalar fx, fy, fz;

	/// pacejka magic formula function, longitudinal
	btScalar PacejkaFx(btScalar sigma, btScalar Fz, btScalar friction_coeff, btScalar & max_Fx) const;

	/// pacejka magic formula function, lateral
	btScalar PacejkaFy(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Fy) const;

	/// pacejka magic formula function, aligning
	btScalar PacejkaMz(btScalar alpha, btScalar Fz, btScalar gamma, btScalar friction_coeff, btScalar & max_Mz) const;

	bool LoadParameters(const PTree & cfg, std::ostream & error);

	void SetDimensions(btScalar width_mm, btScalar ratio_percent, btScalar diameter_in);

	void FindSigmaHatAlphaHat(btScalar load, btScalar & output_sigmahat, btScalar & output_alphahat, int iterations=400);

	void CalculateSigmaHatAlphaHat(int tablesize=20);
};

#endif
