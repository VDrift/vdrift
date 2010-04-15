#ifndef _CAMERA_CHASE_H
#define _CAMERA_CHASE_H

#include "camera.h"

class CAMERA_CHASE : public CAMERA
{
public:
	CAMERA_CHASE(const std::string & name);
	
	virtual MATHVECTOR <float, 3> GetPosition() const
	{
		return position;
	}
	
	virtual QUATERNION <float> GetOrientation() const
	{
		return orientation;
	}
	
	virtual void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing);
	
	virtual void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, const MATHVECTOR <float, 3> & accel, float dt);

	void SetChaseDistance ( float value )
	{
		chase_distance = value;
	}
	
	void SetChaseHeight ( float value )
	{
		chase_height = value;
	}

	void SetPositionBlending ( bool value )
	{
		posblend_on = value;
	}
	
private:
	MATHVECTOR <float, 3> position;
	MATHVECTOR <float, 3> focus;
	QUATERNION <float> orientation;

	float chase_distance;
	float chase_height;
	bool posblend_on;

	float AngleBetween(MATHVECTOR <float, 3> vec1, MATHVECTOR <float, 3> vec2);

	void LookAt(MATHVECTOR <float, 3> eye, MATHVECTOR <float, 3> center, MATHVECTOR <float, 3> up);
};

#endif // _CAMERA_CHASE_H
