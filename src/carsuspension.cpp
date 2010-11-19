#include "carsuspension.h"

template <typename T>
CARSUSPENSIONINFO<T>::CARSUSPENSIONINFO()
{
	spring_constant = 50000.0;
	bounce = 2500;
	rebound = 4000;
	travel = 0.2;
	anti_roll = 8000;
	damper_factors = 1;
	spring_factors = 1;
	max_steering_angle = 0;
	ackermann = 0; 
	camber = 0;
	caster = 0;
	toe = 0;
}

template <typename T>
void CARSUSPENSIONINFO<T>::SetDamperFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
	{
		damper_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
void CARSUSPENSIONINFO<T>::SetSpringFactorPoints(std::vector <std::pair <T, T> > & curve)
{
	for (typename std::vector <std::pair <T, T> >::iterator i = curve.begin(); i != curve.end(); i++)
	{
		spring_factors.AddPoint(i->first, i->second);
	}
}

template <typename T>
CARSUSPENSION<T>::CARSUSPENSION()
{
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
void CARSUSPENSION<T>::Init(const CARSUSPENSIONINFO <T> & info)
{
	this->info = info;
	position = info.extended_position;
	orientation.LoadIdentity();
}

template <typename T>
void CARSUSPENSION<T>::SetSteering(const T & value)
{
	T alpha = -value * info.max_steering_angle * M_PI / 180.0;
	steering_angle = 0.0;
	if(alpha != 0.0)
	{
		steering_angle = atan(1.0 / (1.0 / tan(alpha) - tan(info.ackermann * M_PI / 180.0)));
	}
	
	QUATERNION <T> s;
	s.Rotate(steering_angle, sin(-info.caster * M_PI / 180.0), 0, cos(-info.caster * M_PI / 180.0));
	
	QUATERNION <T> t;
	t.Rotate(info.toe * M_PI / 180.0, 0, 0, 1);
	
	QUATERNION <T> c;
	c.Rotate(-info.camber * M_PI / 180.0, 1, 0, 0);
	
	orientation = c * t * s;
}

template <typename T>
MATHVECTOR <T, 3> CARSUSPENSION<T>::GetWheelPosition(T displacement_fraction) const
{
	//const
	MATHVECTOR <T, 3> relwheelext = info.extended_position - info.hinge;
	MATHVECTOR <T, 3> up (0, 0, 1);
	MATHVECTOR <T, 3> rotaxis = up.cross(relwheelext.Normalize());
	T hingeradius = relwheelext.Magnitude();
	//const
	
	T displacementradians = displacement_fraction * info.travel / hingeradius;
	QUATERNION <T> hingerotate;
	hingerotate.Rotate(-displacementradians, rotaxis[0], rotaxis[1], rotaxis[2]);
	MATHVECTOR <T, 3> localwheelpos = relwheelext;
	hingerotate.RotateVector(localwheelpos);
	
	return localwheelpos + info.hinge;
}

template <typename T>
void CARSUSPENSION<T>::Update(T ext_mass, T ext_velocity, T ext_displacement, T dt)
{
	overtravel = 0;
	displacement = displacement + ext_displacement;
	T velocity = ext_velocity;
	if (displacement > info.travel)
	{
		overtravel = displacement - info.travel;
		displacement = info.travel;
	}
	else if (displacement < 0)
	{
		displacement = 0;
		velocity = 0;
	}
	
	T spring = info.spring_constant;
	T springfactor = info.spring_factors.Interpolate(displacement);
	spring_force = displacement * spring * springfactor; 
	
	T damping = (velocity > 0) ? info.bounce : info.rebound;
	T dampfactor = info.damper_factors.Interpolate(std::abs(velocity));
	damp_force = -velocity * damping * dampfactor;
	
	force = spring_force + damp_force;
	
	// limit damping force (should never add energy)
	T force_limit = -ext_velocity / dt * ext_mass;
	if (force < 0)
	{
		force = 0;
	}
	if (spring_force > force_limit)
	{
		force = spring_force;
	}
	else if (spring_force < force_limit && force > force_limit)
	{
		force = force_limit;
	}
	// suspension bump
	if (force < force_limit && overtravel > 0)
	{
		force = force_limit;
	}
	
	// update wheel position
	position = GetWheelPosition(displacement / info.travel);
}

template <typename T>
void CARSUSPENSION<T>::DebugPrint(std::ostream & out) const
{
	out << "---Suspension---" << "\n";
	out << "Displacement: " << displacement << "\n";
	out << "Force: " << force << "\n";
	out << "Steering angle: " << steering_angle * 180 / M_PI << "\n";
}

template <typename T>
T CARSUSPENSION<T>::GetForce(T displacement, T velocity)
{
	T damping = info.bounce;
	if (velocity < 0) damping = info.rebound;
	
	//compute damper/spring factor based on curve
	T dampfactor = info.damper_factors.Interpolate(std::abs(velocity));
	T springfactor = info.spring_factors.Interpolate(displacement);

	spring_force = -displacement * info.spring_constant * springfactor; 
	damp_force = -velocity * damping * dampfactor;
	T force = spring_force + damp_force;

	return force;
}

/// explicit instantiation
template class CARSUSPENSIONINFO <float>;
template class CARSUSPENSIONINFO <double>;
template class CARSUSPENSION <float>;
template class CARSUSPENSION <double>;
