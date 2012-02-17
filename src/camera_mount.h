#ifndef _CAMERA_MOUNT_H
#define _CAMERA_MOUNT_H

#include "camera.h"
#include "random.h"

class CAMERA_MOUNT : public CAMERA
{
public:
	CAMERA_MOUNT(const std::string & name);

	void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);

	void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt);

	void SetOffset(const MATHVECTOR <float, 3> & lookfrom, const MATHVECTOR <float, 3> & lookat)
	{
		offset = lookfrom;
		offsetrot = LookAt(lookfrom, lookat, direction::Up);
	}

	void SetStiffness(float value)
	{
		stiffness = value;
	}

	void SetEffectStrength(float value)
	{
		offset_effect_strength = value;
	}

private:
	QUATERNION <float> offsetrot;
	MATHVECTOR <float, 3> offset;
	MATHVECTOR <float, 3> displacement;
	MATHVECTOR <float, 3> velocity;
	float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
	float mass;
	float offset_effect_strength; ///< where 1.0 is normal effect strength
	float effect;

	DETERMINISTICRANDOM randgen;

	void UpdatePosition(const MATHVECTOR <float, 3> & newpos);
};

#endif // _CAMERA_MOUNT_H
