#ifndef _CAMERA_FREE_H
#define _CAMERA_FREE_H

#include "camera.h"

class CAMERA_FREE : public CAMERA
{
public:
	CAMERA_FREE(const std::string & name);

	const MATHVECTOR <float, 3> & GetPosition() const {return position;}

	const QUATERNION <float> & GetOrientation() const {return rotation;}

	void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);

	void Rotate(float up, float left);

	void Move(float dx, float dy, float dz);

private:
	MATHVECTOR <float, 3> position;
	QUATERNION <float> rotation;

	MATHVECTOR <float, 3> offset;
	float leftright_rotation;
	float updown_rotation;
};

#endif // _CAMERA_FREE_H
