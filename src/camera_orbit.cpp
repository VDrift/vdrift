#include "camera_orbit.h"
#include "coordinatesystem.h"

CAMERA_ORBIT::CAMERA_ORBIT(const std::string & name) :
	CAMERA(name),
	leftright_rotation(0),
	updown_rotation(0),
	orbit_distance(4.0)
{
	rotation.LoadIdentity();
}

void CAMERA_ORBIT::Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> &)
{
	focus = newfocus;
	leftright_rotation = 0;
	updown_rotation = 0;
	orbit_distance = 4.0;

	rotation.LoadIdentity();
	position = -direction::Forward * orbit_distance;
}

void CAMERA_ORBIT::Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> &, float)
{
	focus = newfocus;
}

void CAMERA_ORBIT::Rotate(float up, float left)
{
	const float maxupdown = 1.5;
	updown_rotation += up;
	if (updown_rotation > maxupdown) updown_rotation = maxupdown;
	if (updown_rotation <-maxupdown) updown_rotation =-maxupdown;

	leftright_rotation -= left;

	rotation.LoadIdentity();
	rotation.Rotate(updown_rotation, direction::Right);
	rotation.Rotate(leftright_rotation, direction::Up);
}

void CAMERA_ORBIT::Move(float dx, float dy, float dz)
{
	MATHVECTOR <float, 3> move(dx, dy, dz);

	orbit_distance -= direction::Forward.dot(move);
	if (orbit_distance < 3.0) orbit_distance = 3.0;
	if (orbit_distance > 10.0) orbit_distance = 10.0;

	position = -direction::Forward * orbit_distance;
	rotation.RotateVector(position);
	position = position + focus;
}
