#ifndef _CAMERA_MOUNT_H
#define _CAMERA_MOUNT_H

#include "camera.h"
#include "rigidbody.h"
#include "random.h"

class CAMERA_MOUNT : public CAMERA
{
public:
	CAMERA_MOUNT(const std::string & name);

	const MATHVECTOR <float, 3> & GetPosition() const {return position;}

	const QUATERNION <float> & GetOrientation() const {return body.GetOrientation();}

	void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);

	void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt);

	void SetOffset(MATHVECTOR <float, 3> value)
	{
		offset = value;
	}

	void SetRotation(float up, float left)
	{
		rotation.Rotate(up, 0, 1, 0);
		rotation.Rotate(left, 0, 0, 1);
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
	MATHVECTOR <float, 3> position;
	QUATERNION <float> rotation; 	// camera rotation relative to car

	RIGIDBODY <float> body;
	MATHVECTOR <float, 3> anchor;
	MATHVECTOR <float, 3> offset;	// offset relative car(center of mass)
	float effect;

	DETERMINISTICRANDOM randgen;

	float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
	float offset_effect_strength; ///< where 1.0 is normal effect strength

	void UpdatePosition();
};

#endif // _CAMERA_MOUNT_H
