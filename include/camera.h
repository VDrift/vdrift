#ifndef _CAMERA_H
#define _CAMERA_H

#include <string>
#include "mathvector.h"
#include "quaternion.h"

///base class for a camera
class CAMERA
{
public:
	CAMERA(const std::string & camera_name) : name(camera_name)
	{
	
	}
	
	const std::string & GetName() const
	{
		return name;
	}
	
	virtual MATHVECTOR <float, 3> GetPosition() const = 0;
	
	virtual QUATERNION <float> GetOrientation() const = 0;
	
	// reset position, orientation
	virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
	
	// update position, orientation
	virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt);
	
	// move relative to current position, orientation
	virtual void Move(float dx, float dy, float dz);
	
	// rotate relative to current position, orientation
	virtual void Rotate(float up, float left);

protected:
	const std::string name;
};

#endif // _CAMERA_H
