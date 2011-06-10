#include "camera_fixed.h"

CAMERA_FIXED::CAMERA_FIXED(const std::string & name) : CAMERA(name)
{
	rotation.LoadIdentity();
}

void CAMERA_FIXED::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	MATHVECTOR <float, 3> newoffset = offset;
	newquat.RotateVector(newoffset);
	position = newpos + newoffset;
	rotation = newquat;
}

void CAMERA_FIXED::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, float)
{
	Reset(newpos, newquat);
}

void CAMERA_FIXED::SetOffset(float x, float y, float z)
{
	offset.Set(x, y, z);
}
