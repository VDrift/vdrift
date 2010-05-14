#ifndef _CAMERA_FREE_H
#define _CAMERA_FREE_H

#include "camera.h"

class CAMERA_FREE : public CAMERA
{
public:
	CAMERA_FREE(const std::string & name);

	virtual MATHVECTOR <float, 3> GetPosition() const;
	
	virtual QUATERNION <float> GetOrientation() const;
	
	virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);

	virtual void Rotate(float up, float left);

	virtual void Move(float dx, float dy, float dz);

private:
	MATHVECTOR <float, 3> position;
	MATHVECTOR <float, 3> offset;
	float leftright_rotation;
	float updown_rotation;
};

#endif // _CAMERA_FREE_H
