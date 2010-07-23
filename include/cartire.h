#ifndef _CARTIRE_H
#define _CARTIRE_H

#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>
#include <vector>

template <typename T>
struct CARTIREINFO
{
	T radius; ///< the total tread radius of the tire in meters
	T aspect_ratio; ///< 0..1 percentage value
	T sidewall_width; /// in meters
	T tread; ///< 1.0 means a pure off-road tire, 0.0 is a pure road tire
	T rolling_resistance_linear; ///< linear rolling resistance on a hard surface
	T rolling_resistance_quadratic; ///< quadratic rolling resistance on a hard surface
	std::vector <T> longitudinal; ///< the parameters of the longitudinal pacejka equation.  this is series b
	std::vector <T> lateral; ///< the parameters of the lateral pacejka equation.  this is series a
	std::vector <T> aligning; ///< the parameters of the aligning moment pacejka equation.  this is series c

	CARTIREINFO();
};

template <typename T>
class CARTIRE
{
friend class joeserialize::Serializer;
public:
	CARTIRE();

	void Init(const CARTIREINFO <T> & info);

	T GetRadius() const	{return info.radius;}
	T GetTread() const	{return info.tread;}
	T GetAspectRatio() const	{return info.aspect_ratio;}
	T GetSidewallWidth() const	{return info.sidewall_width;}

	T GetFeedback() const	{return feedback;}
	T GetSlide() const	{return slide;}
	T GetSlip() const	{return slip;}
	T GetIdealSlide() const	{return ideal_slide;}
	T GetIdealSlip() const	{return ideal_slip;}

	/// Return the friction vector calculated from the magic formula.
	/// velocity: velocity vector of the wheel's reference frame in m/s
	/// ang_velocity: wheel angular veloity in rad/s
	/// inclination: wheel inclination in degrees
	/// normal_force: tire load in newton
	MATHVECTOR <T, 3> GetForce(
		const T normal_force,
		const T friction_coeff,
		const MATHVECTOR <T, 3> & velocity,
		const T ang_velocity,
		const T inclination);

	/// return rolling resistance maximum
	T GetRollingResistance(
		const T velocity,
		const T normal_force,
		const T rolling_resistance_factor) const;

	/// load is the normal force in newtons.
	T GetMaximumFx(T load) const;

	/// load is the normal force in newtons, camber is in degrees
	T GetMaximumFy(T load, T camber) const;

	/// load is the normal force in newtons, camber is in degrees
	T GetMaximumMz(T load, T camber) const;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, feedback);
		return true;
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Tire---" << "\n";
		out << "Inclination: " << camber << "\n";
		out << "Slide ratio: " << slide  << "\n";
		out << "Slip angle: " << slip  << "\n";
	}

private:
	CARTIREINFO <T> info;
	std::vector <T> sigma_hat; ///< maximum grip in the longitudinal direction
	std::vector <T> alpha_hat; ///< maximum grip in the lateral direction

	//for info only
	T feedback; ///< the force feedback effect value
	T camber; ///< tire camber relative to track surface
	T slide; ///< ratio of tire contact patch speed to road speed, minus one
	T slip; ///< the angle (in degrees) between the wheel heading and the wheel's actual velocity
	T ideal_slide; ///< ideal slide ratio
	T ideal_slip; ///< ideal slip angle

	/// look up ideal slide ratio, slip angle
	void LookupSigmaHatAlphaHat(T load, T & sh, T & ah) const;

	/// pacejka magic formula function, longitudinal
	T Pacejka_Fx(T sigma, T Fz, T friction_coeff, T & max_Fx);

	/// pacejka magic formula function, lateral
	T Pacejka_Fy(T alpha, T Fz, T gamma, T friction_coeff, T & max_Fy);

	/// pacejka magic formula function, aligning
	T Pacejka_Mz(T sigma, T alpha, T Fz, T gamma, T friction_coeff, T & max_Mz);

	void FindSigmaHatAlphaHat(T load, T & output_sigmahat, T & output_alphahat, int iterations=400);

	void CalculateSigmaHatAlphaHat(int tablesize=20);
};

#endif
