#ifndef _CAMERA_ORBIT_H
#define _CAMERA_ORBIT_H

#include "camera.h"

class CAMERA_ORBIT : public CAMERA
{
public:
	CAMERA_ORBIT(const std::string & name);
	
	virtual MATHVECTOR <float, 3> GetPosition() const;
	
	virtual QUATERNION <float> GetOrientation() const;

	virtual void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat);

	virtual void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt);
	
	virtual void Rotate(float up, float left);
	
	virtual void Move(float dx, float dy, float dz);

private:
	MATHVECTOR <float, 3> focus;
	float leftright_rotation;
	float updown_rotation;
	float orbit_distance;
};

#endif // _CAMERA_ORBIT_H
