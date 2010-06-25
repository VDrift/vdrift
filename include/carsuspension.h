#ifndef _CARSUSPENSION_H
#define _CARSUSPENSION_H

#include <iostream>

#include "mathvector.h"
#include "quaternion.h"
#include "linearinterp.h"
#include "joeserialize.h"
#include "macros.h"

template <typename T>
class CARSUSPENSION
{
public:
	/// default constructor makes an S2000-like car
	CARSUSPENSION();

	/// spring + damper (const)
	void SetSpringConstant(const T& value)	{spring_constant = value;}
	void SetAntiRollK(const T& value)	{anti_roll_k = value;}
	void SetBounce(const T& value)	{bounce = value;}
	void SetRebound(const T& value)	{rebound = value;}
	void SetTravel(const T& value)	{travel = value;}
	T GetSpringConstant() const	{return spring_constant;}
	T GetAntiRollK() const	{return anti_roll_k;}
	T GetBounce() const	{return bounce;}
	T GetRebound() const	{return rebound;}
	T GetTravel() const	{return travel;}

	/// suspension geometry(const)
	void SetMaxSteeringAngle(const T& value) {max_steering_angle = value;}
	void SetAckermannAngle(const T& value) {ackermann = tan(value);}
	void SetHinge(const MATHVECTOR <T, 3> & value)	{hinge = value;}
	void SetExtendedPosition (const MATHVECTOR <T, 3> & value ) {extended_position = position = value;}
	void SetCamber(const T& value)	{camber = value;}
	void SetCaster(const T& value)	{caster = value;}
	/// for ideal ackemann steering_toe = atan(0.5 * steering_axis_length / axes_distance)
	void SetToe(const T& value)	{toe = value;}
	T GetMaxSteeringAngle() const {return max_steering_angle;}
	const MATHVECTOR <T, 3> & GetExtendedPosition() const {return extended_position;}

	/// steering: -1.0 is maximum right lock and 1.0 is maximum left lock
	void SetSteering(const T& value);

	/// wheel orientation relative to car
	const QUATERNION <T> & GetWheelOrientation() const {return orientation;}
	
	/// wheel position relative to car
	const MATHVECTOR <T, 3> & GetWheelPosition() const {return position;}

	/// displacement: fraction of suspension travel
	MATHVECTOR <T, 3> GetWheelPosition(T displacement) const;

	/// force acting onto wheel
	T GetWheelForce() const {return wheel_force;}
	
	/// suspension force acting onto car body
	T GetForce() const {return force;}
	
	/// relative wheel velocity
	const T GetVelocity() const	{return wheel_velocity;}

	/// suspension displacement: 0.0 fully extended
	const T & GetDisplacement() const	{return displacement;}
	
	T GetDisplacementFraction() const	{return displacement / travel;}
	
	T GetOvertravel() const	{return overtravel;}
	
	/// update wheel position
	void Update(T ext_displacement, T ext_velocity, T dt);

	void SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve);

	void SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve);

	void DebugPrint(std::ostream & out);
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, displacement);
		return true;
	}

friend class joeserialize::Serializer;
private:
	// coilover(const)
	T spring_constant; ///< the suspension spring constant
	T anti_roll_k; ///< the spring constant for the anti-roll bar
	T bounce; ///< suspension compression damping
	T rebound; ///< suspension decompression damping
	T travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped
	LINEARINTERP <T> damper_factors;
	LINEARINTERP <T> spring_factors;

	// suspension geometry(const)
	MATHVECTOR <T, 3> hinge; ///< the point that the wheels are rotated around as the suspension compresses
	MATHVECTOR <T, 3> extended_position; ///< the position of the wheel when the suspension is fully extended (zero g)
	T max_steering_angle; ///< maximum steering angle in degrees
	T ackermann; ///< from -1 to 1, 0.0 means no ackermann effect
	T camber; ///< camber angle in degrees. sign convention depends on the side
	T caster; ///< caster angle in degrees. sign convention depends on the side
	T toe; ///< toe angle in degrees. sign convention depends on the side

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

	void Init();

	T GetForce(T displacement, T velocity);
};

#endif
