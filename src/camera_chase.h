#ifndef _CAMERA_CHASE_H
#define _CAMERA_CHASE_H

#include "camera.h"

class CAMERA_CHASE : public CAMERA
{
public:
	CAMERA_CHASE(const std::string & name);
	
	void SetOffset(const MATHVECTOR <float, 3> & value);

	void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing);

	void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, float dt);

private:
	MATHVECTOR <float, 3> focus;
	MATHVECTOR <float, 3> offset;
	bool posblend_on;
};

#endif // _CAMERA_CHASE_H
