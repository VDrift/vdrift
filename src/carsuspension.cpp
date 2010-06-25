#include "carsuspension.h"

template <typename T>
CARSUSPENSION<T>::CARSUSPENSION()
{
	Init();
}

template <typename T>
void CARSUSPENSION<T>::SetSteering(const T& value)
{
	T alpha = -value * max_steering_angle * M_PI / 180.0;
	steering_angle = 0.0;
	if(alpha != 0.0)
	{
		steering_angle = atan(1.0 / (1.0 / tan(alpha) - ackermann));
	}
	
	QUATERNION <T> s;
	s.Rotate(steering_angle, sin(-caster * M_PI / 180.0), 0, cos(-caster * M_PI / 180.0));
	
	QUATERNION <T> t;
	t.Rotate(toe * M_PI / 180.0, 0, 0, 1);
	
	QUATERNION <T> c;
	c.Rotate(-camber * M_PI / 180.0, 1, 0, 0);
	
	orientation = c * t * s;
}

template <typename T>
MATHVECTOR <T, 3> CARSUSPENSION<T>::GetWheelPosition(T displacement_fraction) const
{
	//const
	MATHVECTOR <T, 3> relwheelext = extended_position - hinge;
	MATHVECTOR <T, 3> up (0, 0, 1);
	MATHVECTOR <T, 3> rotaxis = up.cross(relwheelext.Normalize());
	T hingeradius = relwheelext.Magnitude();
	//const
	
	T displacementradians = displacement_fraction * travel / hingeradius;
	QUATERNION <T> hingerotate;
	hingerotate.Rotate(-displacementradians, rotaxis[0], rotaxis[1], rotaxis[2]);
	MATHVECTOR <T, 3> localwheelpos = relwheelext;
	hingerotate.RotateVector(localwheelpos);
	
	return localwheelpos + hinge;
}

template <typename T>
void CARSUSPENSION<T>::Update(T ext_displacement, T ext_velocity, T dt)
{
	const T inv_wheel_mass = 1 / 20.0; 	// 20kg wheel
	
	// deal with overtravel outside of suspension
	overtravel = ext_displacement + displacement - travel;
	if(overtravel < 0) overtravel = 0;
/*	
	// tire simulate (clamped spring)
	const T tire_stiffness = 3E5;
	T tire_deflection = ext_displacement;
	if (tire_deflection > 0.03) tire_deflection = 0.03;
	else if (tire_deflection < 0.0) tire_deflection = 0.0;
	T tire_force = tire_stiffness * tire_deflection;
	
	// wheel simulation (symplectic euler)
	force = -GetForce(displacement, wheel_velocity - ext_velocity);
	wheel_force = tire_force - force;
	
	T velocity_delta = wheel_force * inv_wheel_mass * dt;
	wheel_velocity += velocity_delta;
	
	T displacement_delta = (wheel_velocity - ext_velocity) * dt;
	displacement += displacement_delta;
	
	// clamp wheel displacement
	if (displacement > travel)
	{
		wheel_velocity = ext_velocity;
		displacement = travel;
	}
	else if (displacement < 0)
	{
		wheel_velocity = ext_velocity;
		displacement = 0;
	}
*/

	// update wheel (symplectic euler)
	wheel_force = GetForce(displacement, wheel_velocity);
	
	T velocity_delta = wheel_force * inv_wheel_mass * dt;
	wheel_velocity += velocity_delta;

	T displacement_delta = wheel_velocity * dt;
	displacement += displacement_delta;

	// external displacement correction
	T force_error = 0;
	T velocity_error = 0;
	T displacement_error = ext_displacement - displacement_delta;
	if (displacement_error > 0)
	{
		if (displacement_error > 0.05) displacement_error = 0.05;
		
		velocity_error = displacement_error / dt;
		force_error = velocity_error / (inv_wheel_mass * dt);
		
		displacement += displacement_error;
		wheel_velocity += velocity_error;
	}
	force = force_error;

	// clamp displacement
	if (displacement > travel)
	{
		wheel_velocity = 0;
		displacement = travel;
	}
	else if (displacement < 0)
	{
		wheel_velocity = 0;
		displacement = 0;
	}

	// update wheel position
	position = GetWheelPosition(displacement/travel);
}

template <typename T>
void CARSUSPENSION<T>::SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
	{
		damper_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
void CARSUSPENSION<T>::SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
	{
		spring_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
void CARSUSPENSION<T>::DebugPrint(std::ostream & out)
{
	out << "---Suspension---" << "\n";
	out << "Displacement: " << displacement << "\n";
	out << "Force: " << force << "\n";
	//out << "Velocity: " << wheel_velocity << "\n";
	//out << "Wheel force: " << wheel_force << "\n";
	//out << "Damp force: " << damp_force << "\n";
	out << "Steering angle: " << steering_angle * 180 / M_PI << "\n";
}

template <typename T>
void CARSUSPENSION<T>::Init()
{
	spring_constant = 50000.0;
	bounce = 2500;
	rebound = 4000;
	travel = 0.2;
	anti_roll_k = 8000;
	damper_factors = 1;
	spring_factors = 1;
	
	camber = 0;
	caster = 0;
	toe = 0;
	
	orientation.LoadIdentity();
	steering_angle = 0;
	spring_force = 0;
	damp_force = 0;
	force = 0;
	
	overtravel = 0;
	displacement = 0;
	wheel_velocity = 0;
	wheel_force = 0;
}

template <typename T>
T CARSUSPENSION<T>::GetForce(T displacement, T velocity)
{
	T damping = bounce;
	if (velocity < 0) damping = rebound;
	
	//compute damper/spring factor based on curve
	T dampfactor = damper_factors.Interpolate(std::abs(velocity));
	T springfactor = spring_factors.Interpolate(displacement);

	spring_force = -displacement * spring_constant * springfactor; 
	damp_force = -velocity * damping * dampfactor;
	T force = spring_force + damp_force;

	return force;
}

/// explicit instantiation
template class CARSUSPENSION <float>;
template class CARSUSPENSION <double>;
