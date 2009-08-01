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
	T max_compression_velocity; ///< the absolute maximum speed that the suspension is allowed to compress or decompress at
	T travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped
	T anti_roll_k; ///< the spring constant for the anti-roll bar
	T camber; ///< camber angle in degrees. sign convention depends on the side
	T caster; ///< caster angle in degrees. sign convention depends on the side
	T toe; ///< toe angle in degrees. sign convention depends on the side
	//MATHVECTOR <T, 3> position; ///< where the suspension applies its force on the car body
	LINEARINTERP <T> damper_factors;
	LINEARINTERP <T> spring_factors;
	
	//variables
	T displacement; ///< a linear representation of the suspension displacement.  in actuality the displacement is about the arc formed by the hinge
	T last_displacement; ///< the displacement one frame ago
	T overtravel; ///< the amount past the travel that the suspension was requested to compress
	//T last_last_displacement; ///< the displacement two frames ago
	
	//for info only
	mutable T velocity;
	mutable T damp_force;
	mutable T spring_force;
	mutable T antiroll_force;
	
public:
	//default constructor makes an S2000-like car
	CARSUSPENSION() : spring_constant(50000.0), bounce(2588), rebound(2600), travel(0.19),
			anti_roll_k(8000), camber(-0.5), caster(0.28), toe(0),
			damper_factors(1), spring_factors(1), displacement(0),
			last_displacement(0), overtravel(0),
			velocity(0), damp_force(0), spring_force(0) {}

	void DebugPrint(std::ostream & out)
	{
		out << "---Suspension---" << std::endl;
		out << "Displacement: " << displacement << std::endl;
		out << "Velocity: " << velocity << std::endl;
		out << "Spring force: " << spring_force << std::endl;
		out << "Spring factor: " << spring_factors.Interpolate(displacement) << std::endl;
		out << "Damp force: " << damp_force << std::endl;
		out << "Damp factor: " << damper_factors.Interpolate(std::abs(velocity)) << std::endl;
		out << "Anti-roll force: " << antiroll_force << std::endl;
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

	/*void SetPosition ( const MATHVECTOR< T, 3 >& value )
	{
		position = value;
	}
	

	MATHVECTOR< T, 3 > GetPosition() const
	{
		return position;
	}*/

	void SetSpringConstant ( const T& value )
	{
		spring_constant = value;
	}
	

	T GetSpringConstant() const
	{
		return spring_constant;
	}

	void SetDisplacement ( const T& value, T dt )
	{
		//last_last_displacement = last_displacement;
		last_displacement = displacement;
		displacement = value;

		if (displacement > travel)
			displacement = travel;
		if (displacement < 0)
			displacement = 0;
		
		overtravel = value - travel;
		if (overtravel < 0)
			overtravel = 0;
		
		//enforce maximum compression velocity
		/*if (displacement - last_displacement < -max_compression_velocity*dt)
			displacement = last_displacement - max_compression_velocity*dt;
		if (displacement - last_displacement > max_compression_velocity*dt)
			displacement = last_displacement + max_compression_velocity*dt;*/
	}
	

	const T & GetDisplacement() const
	{
		return displacement;
	}
	
	///Return the displacement in percent of max travel where 0.0 is fully extended and 1.0 is fully compressed
	T GetDisplacementPercent() const
	{
		return displacement/travel;
	}
	
	const T & GetLastDisplacement() const
	{
		return last_displacement;
	}
	
	///compute the suspension force for the given time interval
	T GetForce(T dt) const
	{
		T damping = bounce;
		//note that displacement is defined opposite to the classical definition (positive values mean compressed instead of extended)
		velocity = (displacement - last_displacement)/dt;
		if (velocity < 0)
		{
			damping = rebound;
		}
		
		//compute damper factor based on curve
		T velabs = std::abs(velocity);
		T dampfactor = damper_factors.Interpolate(velabs);
		
		//compute spring factor based on curve
		T springfactor = spring_factors.Interpolate(displacement);
		
		spring_force = displacement * spring_constant * springfactor; //when compressed, the spring force will push the car in the positive z direction
		damp_force = velocity * damping * dampfactor; //when compression is increasing, the damp force will push the car in the positive z direction
		return spring_force + damp_force;
	}

	const T & GetVelocity() const
	{
		return velocity;
	}
	
	const T & GetDampForce() const
	{
		return damp_force;
	}
	
	const T & GetSpringForce() const
	{
		return spring_force;
	}

	void SetMaxCompressionVelocity ( const T& value )
	{
		max_compression_velocity = value;
	}
	
	void SetAntiRollInfo(const T value)
	{
		antiroll_force = value;
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,displacement);
		_SERIALIZE_(s,last_displacement);
		return true;
	}

	T GetOvertravel() const
	{
		return overtravel;
	}
	
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
