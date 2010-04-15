#include "camera_free.h"

CAMERA_FREE::CAMERA_FREE(const std::string & name)
: CAMERA(name), offset(0, 0, 2), leftright_rotation(0), updown_rotation(0)
{

}

MATHVECTOR <float, 3> CAMERA_FREE::GetPosition() const
{
	return position;
}

QUATERNION <float> CAMERA_FREE::GetOrientation() const
{
	QUATERNION <float> rot;
	rot.Rotate(updown_rotation, 0, 1, 0);
	rot.Rotate(leftright_rotation, 0, 0, 1);
	return rot;
}

void CAMERA_FREE::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> &)
{
	position = newpos + offset;
	leftright_rotation = 0;
	updown_rotation = 0;
	Move(-8.0, 0, 0);
}

void CAMERA_FREE::Rotate(float up, float left)
{
	updown_rotation += up;
	if (updown_rotation > 1.0)
		updown_rotation = 1.0;
	if (updown_rotation <-1.0)
		updown_rotation =-1.0;
	
	leftright_rotation += left;
}

void CAMERA_FREE::Move(float dx, float, float)
{
	MATHVECTOR <float, 3> forward(dx, 0, 0);
	GetOrientation().RotateVector(forward);
	position = position + forward;
}
