#ifndef _CAMERA_FIXED_H
#define _CAMERA_FIXED_H

#include "camera.h"

class CAMERA_FIXED : public CAMERA
{
public:
	CAMERA_FIXED(const std::string & name);
	
	virtual MATHVECTOR <float, 3> GetPosition() const
	{
		return position;
	}
	
	virtual QUATERNION <float> GetOrientation() const
	{
		return orientation;
	}

	virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
	
	virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt);

	void SetOffset(float x, float y, float z);

private:
	MATHVECTOR <float, 3> position;
	MATHVECTOR <float, 3> offset;
	QUATERNION <float> orientation;
};

#endif // _CAMERA_FIXED_H
