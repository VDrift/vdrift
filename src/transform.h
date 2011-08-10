#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "quaternion.h"
#include "mathvector.h"

class TRANSFORM
{
public:
	typedef QUATERNION<float> QUAT;
	typedef MATHVECTOR<float,3> VEC3;

	const QUAT & GetRotation() const {return rotation;}
	const VEC3 & GetTranslation() const {return translation;}
	void SetRotation(const QUAT & rot) {rotation = rot;}
	void SetTranslation(const VEC3 & trans) {translation = trans;}
	bool IsIdentityTransform() const {return (rotation == QUAT() && translation == VEC3());}
	void Clear() {rotation.LoadIdentity();translation.Set(0.0f);}

private:
	QUAT rotation;
	VEC3 translation;
};

#endif // _TRANSFORM_H
