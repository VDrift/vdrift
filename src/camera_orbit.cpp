#include "camera_orbit.h"

CAMERA_ORBIT::CAMERA_ORBIT(const std::string & name)
: CAMERA(name), leftright_rotation(0), updown_rotation(0), orbit_distance(4.0)
{

}

MATHVECTOR <float, 3> CAMERA_ORBIT::GetPosition() const
{
	MATHVECTOR <float, 3> position(-orbit_distance,0,0);
	GetOrientation().RotateVector(position);
	return focus + position;
}

QUATERNION <float> CAMERA_ORBIT::GetOrientation() const
{
	QUATERNION <float> rot;
	rot.Rotate(updown_rotation, 0, 1, 0);
	rot.Rotate(leftright_rotation, 0, 0, 1);
	return rot;
}

void CAMERA_ORBIT::Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> &)
{
	focus = newfocus;
	leftright_rotation = 0;
	updown_rotation = 0;
	orbit_distance = 4.0;
}

void CAMERA_ORBIT::Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> &, const MATHVECTOR <float, 3> &, float)
{
	focus = newfocus;
}

void CAMERA_ORBIT::Rotate(float up, float left)
{
	const float maxupdown = 1.5;
	updown_rotation += up;
	if (updown_rotation > maxupdown)
		updown_rotation = maxupdown;
	if (updown_rotation <-maxupdown)
		updown_rotation =-maxupdown;
	
	leftright_rotation -= left;
}

void CAMERA_ORBIT::Move(float dx, float, float)
{
	orbit_distance -= dx;
	if (orbit_distance < 3.0)
		orbit_distance = 3.0;
	if (orbit_distance > 10.0)
		orbit_distance = 10.0;
}
