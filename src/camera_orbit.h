#ifndef _CAMERA_ORBIT_H
#define _CAMERA_ORBIT_H

#include "camera.h"

class CAMERA_ORBIT : public CAMERA
{
public:
	CAMERA_ORBIT(const std::string & name);
	
	void SetOffset(const MATHVECTOR <float, 3> & value);

	void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat);

	void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat, float dt);

	void Rotate(float up, float left);

	void Move(float dx, float dy, float dz);

private:
	MATHVECTOR <float, 3> focus;
	MATHVECTOR <float, 3> offset;
	float distance;
	float leftright_rotation;
	float updown_rotation;
};

#endif // _CAMERA_ORBIT_H
