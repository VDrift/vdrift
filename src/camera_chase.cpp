#include "camera_chase.h"
#include "coordinatesystem.h"

CAMERA_CHASE::CAMERA_CHASE(const std::string & name) :
	CAMERA(name),
	chase_distance(6),
	chase_height(1.5),
	posblend_on(true)
{
	rotation.LoadIdentity();
}

void CAMERA_CHASE::Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing)
{
	focus = newfocus;
	rotation = focus_facing;
	MATHVECTOR <float, 3> view_offset = -direction::Forward * chase_distance + direction::Up * chase_height;
	rotation.RotateVector(view_offset);
	position = focus + view_offset;
}

void CAMERA_CHASE::Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, float dt)
{
	focus = newfocus;
	MATHVECTOR <float, 3> view_offset = -direction::Forward * chase_distance + direction::Up * chase_height;
	focus_facing.RotateVector(view_offset);
	MATHVECTOR <float, 3> target_position = focus + view_offset;

	float posblend = 10.0 * dt;
	if (posblend > 1.0) posblend = 1.0;
	if (!posblend_on) posblend = 1.0;
	position = position * (1.0 - posblend) + target_position * posblend;

	MATHVECTOR <float, 3> focus_offset(0, 0, 0);
	if (chase_distance < 0.0001)
	{
		focus_offset = direction::Forward;
		focus_facing.RotateVector(focus_offset);
	}

	LookAt(position, focus + focus_offset, direction::Up);
}

void CAMERA_CHASE::LookAt(MATHVECTOR <float, 3> eye, MATHVECTOR <float, 3> center, MATHVECTOR <float, 3> up)
{
	MATHVECTOR <float, 3> forward(center - eye);
	forward = forward.Normalize();
	MATHVECTOR <float, 3> side = (forward.cross(up)).Normalize();
	MATHVECTOR <float, 3> realup = side.cross(forward);

	//rotate so the camera is pointing along the forward line
	float theta = AngleBetween(forward, direction::Forward);
	assert(theta == theta);
	rotation.LoadIdentity();
	MATHVECTOR <float, 3> axis = forward.cross(direction::Forward).Normalize();
	rotation.Rotate(-theta, axis[0], axis[1], axis[2]);

	//now rotate the camera so it's pointing up
	MATHVECTOR <float, 3> curup = direction::Up;
	rotation.RotateVector(curup);
	
	float rollangle = AngleBetween(realup, curup);
	if (curup.dot(side) > 0.0)
	{
		const float pi = 3.141593;
		rollangle = (pi - rollangle) + pi;
	}
	
	axis = forward;
	rotation.Rotate(rollangle, axis[0], axis[1], axis[2]);

	assert(rollangle == rollangle);
}

float CAMERA_CHASE::AngleBetween(MATHVECTOR <float, 3> vec1, MATHVECTOR <float, 3> vec2)
{
	float dotprod = vec1.Normalize().dot(vec2.Normalize());
	float angle = acos(dotprod);
	float epsilon = 1e-6;
	if (fabs(dotprod) <= epsilon) angle = 3.141593 * 0.5;
	if (dotprod >= 1.0-epsilon) angle = 0.0;
	if (dotprod <= -1.0+epsilon) angle = 3.141593;
	return angle;
}
