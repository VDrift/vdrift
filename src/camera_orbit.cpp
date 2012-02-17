#include "camera_orbit.h"
#include "coordinatesystem.h"

CAMERA_ORBIT::CAMERA_ORBIT(const std::string & name) :
	CAMERA(name),
	offset(-direction::Forward * 3),
	distance(-1.5 * direction::Forward.dot(offset)),
	leftright_rotation(0),
	updown_rotation(0)
{
	rotation.LoadIdentity();
}

void CAMERA_ORBIT::SetOffset(const MATHVECTOR <float, 3> & value)
{
	offset = value;
	if (offset.dot(direction::Forward) > -0.001)
	{
		offset = offset - direction::Forward;
	}
}

void CAMERA_ORBIT::Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> &)
{
	focus = newfocus;
	distance = -1.5 * direction::Forward.dot(offset);
	leftright_rotation = 0;
	updown_rotation = 0;
	rotation.LoadIdentity();
	position = offset;
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
	distance += direction::Forward.dot(move);
	float min_distance = -direction::Forward.dot(offset);
	float max_distance = 4 * min_distance;
	if (distance < min_distance) distance = min_distance;
	else if (distance > max_distance) distance = max_distance;

	position = -direction::Forward * distance;
	rotation.RotateVector(position);
	position = position + focus;
}
