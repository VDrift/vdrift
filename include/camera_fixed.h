#ifndef _CAMERA_FIXED_H
#define _CAMERA_FIXED_H

#include "camera.h"

class CAMERA_FIXED : public CAMERA
{
public:
	CAMERA_FIXED(const std::string & name);
	
	const MATHVECTOR <float, 3> & GetPosition() const {return position;}
	
	const QUATERNION <float> & GetOrientation() const {return rotation;}

	void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
	
	void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, float dt);

	void SetOffset(float x, float y, float z);

private:
	MATHVECTOR <float, 3> position;
	QUATERNION <float> rotation;
	
	MATHVECTOR <float, 3> offset;
	
};

#endif // _CAMERA_FIXED_H
