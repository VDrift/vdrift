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
	T force;
	
	// wheel
	T overtravel;
	T wheel_displacement;
	T wheel_velocity;
	T wheel_force;
	T tire_deflection;

public:
	//default constructor makes an S2000-like car
	CARSUSPENSION() :
		spring_constant(50000.0), bounce(2588), rebound(2600), travel(0.19),
		anti_roll_k(8000), damper_factors(1), spring_factors(1),
		camber(-0.5), caster(0.28), toe(0),
		spring_force(0), damp_force(0), force(0),
		wheel_displacement(0), wheel_velocity(0), wheel_force(0), tire_deflection(0)
	{
		
	}

	void DebugPrint(std::ostream & out)
	{
		out << "---Suspension---" << std::endl;
		out << "Displacement: " << wheel_displacement << std::endl;
		//out << "Velocity: " << wheel_velocity << std::endl;
		//out << "Force: " << force << std::endl;
		out << "Spring force: " << spring_force << std::endl;
		out << "Damp force: " << damp_force << std::endl;
		//out << "Spring factor: " << spring_factors.Interpolate(displacement) << std::endl;
		//out << "Damp factor: " << damper_factors.Interpolate(std::abs(velocity)) << std::endl;
	}

	void SetHinge ( const MATHVECTOR< T, 3 >& value )
	{
		hinge = value;
	}

	const MATHVECTOR< T, 3 > & GetHinge() const
	{
		return hinge;
	}

	void SetBounce ( const T& value )
	{
		bounce = value;
	}

	T GetBounce() const
	{
		return bounce;
	}

	void SetRebound ( const T& value )
	{
		rebound = value;
	}

	T GetRebound() const
	{
		return rebound;
	}

	void SetTravel ( const T& value )
	{
		travel = value;
	}

	T GetTravel() const
	{
		return travel;
	}

	void SetAntiRollK ( const T& value )
	{
		anti_roll_k = value;
	}

	T GetAntiRollK() const
	{
		return anti_roll_k;
	}

	void SetCamber ( const T& value )
	{
		camber = value;
	}

	T GetCamber() const
	{
		return camber;
	}

	void SetCaster ( const T& value )
	{
		caster = value;
	}

	T GetCaster() const
	{
		return caster;
	}

	void SetToe ( const T& value )
	{
		toe = value;
	}

	T GetToe() const
	{
		return toe;
	}

	void SetSpringConstant ( const T& value )
	{
		spring_constant = value;
	}

	T GetSpringConstant() const
	{
		return spring_constant;
	}

	const T & GetDisplacement() const
	{
		return wheel_displacement;
	}

	///Return the displacement in percent of max travel where 0.0 is fully extended and 1.0 is fully compressed
	T GetDisplacementPercent() const
	{
		return wheel_displacement / travel;
	}

	// road_displacement(relative to wheel)
	T Update(T dt, T road_displacement)
	{
		// wheel properties
		const T inv_wheel_mass = 1 / 20.0; 	// 20kg wheel
		const T tire_stiffness = 2E5;		// [N/m] http://www.hunter.com/pub/undercar/SAEATS13/
		const T tire_deflection_max = 0.1;	// [m] maximum vertical tire deflection
		const T bump_stiffness = 5E5;		// hard rubber bump (todo: replace with bullet constraint)
		
		// clamp tire sidewall deflection
		T tire_deflection = road_displacement;
		if (tire_deflection > tire_deflection_max)
			tire_deflection = tire_deflection_max;
		else if (tire_deflection < 0)
			tire_deflection = 0;
		
		// update wheel velocity
		wheel_velocity = wheel_velocity + wheel_force * inv_wheel_mass * dt;
		
		// update wheel displacement
		wheel_displacement = wheel_displacement + wheel_velocity * dt;
		
		// clamp wheel displacement
		T bump_force = 0;
		if (wheel_displacement > travel)
		{
			bump_force += bump_stiffness * (wheel_displacement - travel);
			wheel_displacement = travel;
			wheel_velocity = 0;
		}
		else if (wheel_displacement < 0)
		{
			wheel_displacement = 0;
			wheel_velocity = 0;
		}
		
		// update wheel force
		force = GetForce(wheel_displacement, wheel_velocity) + bump_force;
		wheel_force = -force + tire_stiffness * tire_deflection;
		
		return force;
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

	const T & GetVelocity() const
	{
		return wheel_velocity;
	}
/*
	void SetAntiRollInfo(const T value)
	{
		antiroll_force = value;
	}
*/
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, wheel_displacement);
		return true;
	}
/*
	T GetOvertravel() const
	{
		return overtravel;
	}
*/
	void SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve)
	{
		//std::cout << "Damper factors: " << std::endl;
		for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
		{
			//std::cout << i->first << ", " << i->second << std::endl;
			damper_factors.AddPoint(i->first, i->second);
		}
	}

	void SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve)
	{
		//std::cout << "Spring factors: " << std::endl;
		for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
		{
			//std::cout << i->first << ", " << i->second << std::endl;
			spring_factors.AddPoint(i->first, i->second);
		}
	}
};

#endif
