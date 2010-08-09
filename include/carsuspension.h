#ifndef _CARSUSPENSION_H
#define _CARSUSPENSION_H

#include <iostream>

#include "mathvector.h"
#include "quaternion.h"
#include "linearinterp.h"
#include "joeserialize.h"
#include "macros.h"

template <typename T>
struct CARSUSPENSIONINFO
{
	// coilover(const)
	T spring_constant; ///< the suspension spring constant
	T anti_roll; ///< the spring constant for the anti-roll bar
	T bounce; ///< suspension compression damping
	T rebound; ///< suspension decompression damping
	T travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped
	LINEARINTERP <T> damper_factors;
	LINEARINTERP <T> spring_factors;

	// suspension geometry(const)
	MATHVECTOR <T, 3> hinge; ///< the point that the wheels are rotated around as the suspension compresses
	MATHVECTOR <T, 3> extended_position; ///< the position of the wheel when the suspension is fully extended (zero g)
	T max_steering_angle; ///< maximum steering angle in degrees
	T ackermann; ///< /// for ideal ackemann steering_toe = atan(0.5 * steering_axis_length / axes_distance)
	T camber; ///< camber angle in degrees. sign convention depends on the side
	T caster; ///< caster angle in degrees. sign convention depends on the side
	T toe; ///< toe angle in degrees. sign convention depends on the side

	CARSUSPENSIONINFO(); ///< default constructor makes an S2000-like car

	void SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve);

	void SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve);
};

template <typename T>
class CARSUSPENSION
{
public:
	CARSUSPENSION();

	void Init(const CARSUSPENSIONINFO <T> & info);

	const T & GetAntiRoll() const	{return info.anti_roll;}

	const T & GetMaxSteeringAngle() const {return info.max_steering_angle;}

	/// wheel orientation relative to car
	const QUATERNION <T> & GetWheelOrientation() const {return orientation;}

	/// wheel position relative to car
	const MATHVECTOR <T, 3> & GetWheelPosition() const {return position;}

	/// displacement: fraction of suspension travel
	MATHVECTOR <T, 3> GetWheelPosition(T displacement) const;

	/// force acting onto wheel
	const T & GetWheelForce() const {return wheel_force;}

	/// suspension force acting onto car body
	const T & GetForce() const {return force;}

	/// relative wheel velocity
	const T & GetVelocity() const	{return wheel_velocity;}

	/// wheel overtravel
	const T & GetOvertravel() const	{return overtravel;}

	/// wheel displacement
	const T & GetDisplacement() const {return displacement;}

	/// displacement fraction: 0.0 fully extended, 1.0 fully compressed
	T GetDisplacementFraction() const	{return displacement / info.travel;}

	/// steering: -1.0 is maximum right lock and 1.0 is maximum left lock
	void SetSteering(const T & value);

	/// update wheel position
	void Update(T ext_displacement, T ext_velocity, T dt);

	void DebugPrint(std::ostream & out) const;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, displacement);
		return true;
	}

friend class joeserialize::Serializer;
private:
	CARSUSPENSIONINFO <T> info;

	// suspension
	QUATERNION <T> orientation;
	MATHVECTOR <T, 3> position;
	T steering_angle;
	T spring_force;
	T damp_force;
	T force;

	// wheel
	T overtravel;
	T displacement;
	T wheel_velocity;
	T wheel_force;

	T GetForce(T displacement, T velocity);
};

#endif
