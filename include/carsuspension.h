#ifndef _CARSUSPENSION_H
#define _CARSUSPENSION_H

#include <iostream>
#include <cmath>

#include "mathvector.h"
#include "linearframe.h"
#include "joeserialize.h"
#include "macros.h"
#include "linearinterp.h"

template <typename T>
class CARSUSPENSION
{
public:
	//default constructor makes an S2000-like car
	CARSUSPENSION()	{Init();}

	void SetHinge(const MATHVECTOR <T, 3> & value)	{hinge = value;}
	void SetSpringConstant(const T& value)	{spring_constant = value;}
	void SetAntiRollK(const T& value)	{anti_roll_k = value;}
	void SetBounce(const T& value)	{bounce = value;}
	void SetRebound(const T& value)	{rebound = value;}
	void SetTravel(const T& value)	{travel = value;}
	void SetCamber(const T& value)	{camber = value;}
	void SetCaster(const T& value)	{caster = value;}
	void SetToe(const T& value)	{toe = value;}

	const MATHVECTOR <T, 3> & GetHinge() const	{return hinge;}
	T GetSpringConstant() const	{return spring_constant;}
	T GetAntiRollK() const	{return anti_roll_k;}
	T GetBounce() const	{return bounce;}
	T GetRebound() const	{return rebound;}
	T GetTravel() const	{return travel;}
	T GetCamber() const	{return camber;}
	T GetCaster() const	{return caster;}
	T GetToe() const	{return toe;}

	/// force acting onto wheel
	T GetWheelForce() const {return wheel_force;}
	
	/// suspension force acting onto car body
	T GetForce() const {return force;}

	/// suspension displacement: 0.0 fully extended
	const T & GetDisplacement() const	{return wheel_displacement;}
	T GetDisplacementFraction() const	{return wheel_displacement / travel;}
	
	/// get suspension overtravel for constraint solver
	T GetOvertravel() const	{return overtravel;}

	/// ext_displacement: relative to wheel/tire
	void Update(T ext_displacement, T dt)
	{
		// wheel properties
		const T inv_wheel_mass = 1 / 20.0; 	// 20kg wheel

		// deal with overtravel outside of suspension
		overtravel = ext_displacement + wheel_displacement - travel;
		if(overtravel < 0) overtravel = 0;

		// clamp external displacement
		ext_displacement = ext_displacement - overtravel;
		
		// update wheel (symplectic euler)
		wheel_force = -GetForce(wheel_displacement, wheel_velocity);
		
		T velocity_delta = wheel_force * inv_wheel_mass * dt;
		wheel_velocity += velocity_delta;

		T displacement_delta = wheel_velocity * dt;
		wheel_displacement += displacement_delta;

		// external displacement correction
		T force_error = 0;
		T velocity_error = 0;
		T displacement_error = ext_displacement - displacement_delta;
		if (displacement_error > 0)
		{
			velocity_error = displacement_error / dt;
			force_error = velocity_error / (inv_wheel_mass * dt);
			
			wheel_displacement += displacement_error;
			wheel_velocity += velocity_error;
		}
		
		// clamp wheel displacement
		if (wheel_displacement > travel)
		{
			wheel_velocity = 0;
			wheel_displacement = travel;
		}
		else if (wheel_displacement < 0)
		{
			wheel_velocity = 0;
			wheel_displacement = 0;
		}
		
		force = force_error;
	}

	const T GetVelocity() const
	{
		return wheel_velocity;
	}

	void SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve)
	{
		for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
		{
			damper_factors.AddPoint(i->first, i->second);
		}
	}

	void SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve)
	{
		for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
		{
			spring_factors.AddPoint(i->first, i->second);
		}
	}

	void DebugPrint(std::ostream & out)
	{
		out << "---Suspension---" << "\n";
		out << "Displacement: " << wheel_displacement << "\n";
		out << "Normal force: " << force << "\n";
		out << "Velocity: " << wheel_velocity << "\n";
		//out << "Spring force: " << spring_force << "\n";
		out << "Damp force: " << damp_force << "\n";
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, wheel_displacement);
		return true;
	}

friend class joeserialize::Serializer;
private:
	//constants (not actually declared as const because they can be changed after object creation)
	MATHVECTOR <T, 3> hinge; ///< the point that the wheels are rotated around as the suspension compresses
	T spring_constant; ///< the suspension spring constant
	T bounce; ///< suspension compression damping
	T rebound; ///< suspension decompression damping
	T travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped
	T anti_roll_k; ///< the spring constant for the anti-roll bar
	LINEARINTERP <T> damper_factors;
	LINEARINTERP <T> spring_factors;

	T camber; ///< camber angle in degrees. sign convention depends on the side
	T caster; ///< caster angle in degrees. sign convention depends on the side
	T toe; ///< toe angle in degrees. sign convention depends on the side

	// suspension
	T spring_force;
	T damp_force;
	T force;	// damp_force + spring_force

	// wheel
	T overtravel;
	T wheel_displacement;
	T wheel_velocity;
	T wheel_force;

	void Init()
	{
		spring_constant = 50000.0;
		bounce = 2588;
		rebound = 2600;
		travel = 0.19;
		anti_roll_k = 8000;
		damper_factors = 1;
		spring_factors = 1;
		camber = -0.5;
		caster = 0.28;
		toe = 0;
		spring_force = 0;
		damp_force = 0;
		force = 0;
		overtravel = 0;
		wheel_displacement = 0;
		wheel_velocity = 0;
		wheel_force = 0;
	}

	const T GetForce(T displacement, T velocity)
	{
		T damping = bounce;
		if (velocity < 0) damping = rebound;
		
		//compute damper/spring factor based on curve
		T dampfactor = damper_factors.Interpolate(std::abs(velocity));
		T springfactor = spring_factors.Interpolate(displacement);

		spring_force = displacement * spring_constant * springfactor; 
		damp_force = velocity * damping * dampfactor;
		T force = spring_force + damp_force;

		return force;
	}
};

#endif
