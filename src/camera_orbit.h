#ifndef _CAMERA_ORBIT_H
#define _CAMERA_ORBIT_H

#include "camera.h"

class CAMERA_ORBIT : public CAMERA
{
public:
	CAMERA_ORBIT(const std::string & name);

	const MATHVECTOR <float, 3> & GetPosition() const {return position;}

	const QUATERNION <float> & GetOrientation() const {return rotation;}

	void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat);

	void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat, float dt);

	void Rotate(float up, float left);

	void Move(float dx, float dy, float dz);

private:
	MATHVECTOR <float, 3> position;
	QUATERNION <float> rotation;

	MATHVECTOR <float, 3> focus;
	float leftright_rotation;
	float updown_rotation;
	float orbit_distance;
};

#endif // _CAMERA_ORBIT_H
