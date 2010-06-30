#ifndef _RIGIDBODY_H
#define _RIGIDBODY_H

#include "linearframe.h"
#include "rotationalframe.h"
#include "mathvector.h"
#include "quaternion.h"
#include "joeserialize.h"
#include "macros.h"

template <typename T>
class RIGIDBODY
{
friend class joeserialize::Serializer;
private:
	LINEARFRAME <T> linear;
	ROTATIONALFRAME <T> rotation;
	
public:
	//access to linear frame
	void SetInitialForce(const MATHVECTOR <T, 3> & force) {linear.SetInitialForce(force);}
	void SetForce(const MATHVECTOR <T, 3> & force) {linear.SetForce(force);}
	void SetMass(const T & mass) {linear.SetMass(mass);}
	const T GetMass() const {return linear.GetMass();}
	void SetPosition(const MATHVECTOR <T, 3> & position) {linear.SetPosition(position);}
	const MATHVECTOR <T, 3> & GetPosition() const {return linear.GetPosition();}
	void SetVelocity(const MATHVECTOR <T, 3> & velocity) {linear.SetVelocity(velocity);}
	const MATHVECTOR <T, 3> GetVelocity() const {return linear.GetVelocity();}
	const MATHVECTOR <T, 3> GetVelocity(const MATHVECTOR <T, 3> & offset) {return linear.GetVelocity() + rotation.GetAngularVelocity().cross(offset);}
	const MATHVECTOR <T, 3> & GetForce() const {return linear.GetForce();}
	
	//access to rotational frame
	void SetInitialTorque(const MATHVECTOR <T, 3> & torque) {rotation.SetInitialTorque(torque);}
	void SetTorque(const MATHVECTOR <T, 3> & torque) {rotation.SetTorque(torque);}
	void SetInertia(const MATRIX3 <T> & inertia) {rotation.SetInertia(inertia);}
	const MATRIX3 <T> & GetInertia() {return rotation.GetInertia();}
	const MATRIX3 <T> & GetWorldInertia() {return rotation.GetWorldInertia();}
	void SetOrientation(const QUATERNION <T> & orientation) {rotation.SetOrientation(orientation);}
	const QUATERNION <T> & GetOrientation() const {return rotation.GetOrientation();}
	void SetAngularVelocity(const MATHVECTOR <T, 3> & newangvel) {rotation.SetAngularVelocity(newangvel);}
	const MATHVECTOR <T, 3> GetAngularVelocity() const {return rotation.GetAngularVelocity();}
	
	//acessing both linear and rotational frames
	void Integrate1(const T & dt) {linear.Integrate1(dt);rotation.Integrate1(dt);}
	void Integrate2(const T & dt) {linear.Integrate2(dt);rotation.Integrate2(dt);}
	const MATHVECTOR <T, 3> TransformLocalToWorld(const MATHVECTOR <T, 3> & localpoint) const
	{
		MATHVECTOR <T, 3> output(localpoint);
		GetOrientation().RotateVector(output);
		output = output + GetPosition();
		return output;
	}
	const MATHVECTOR <T, 3> TransformWorldToLocal(const MATHVECTOR <T, 3> & worldpoint) const
	{
		MATHVECTOR <T, 3> output(worldpoint);
		output = output - GetPosition();
		(-GetOrientation()).RotateVector(output);
		return output;
	}
	
	// apply force in world space
	void ApplyForce(const MATHVECTOR <T, 3> & force)
	{
		linear.ApplyForce(force);
	}
	
	// apply force at offset from center of mass in world space
	void ApplyForce(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset)
	{
		linear.ApplyForce(force);
		rotation.ApplyTorque(offset.cross(force));
	}
	
	// apply torque in world space
	void ApplyTorque(const MATHVECTOR <T, 3> & torque)
	{
		rotation.ApplyTorque(torque);
	}
	
	// inverse of the effective mass at offset, along normal
	T GetInvEffectiveMass(const MATHVECTOR <T, 3> & normal, const MATHVECTOR <T, 3> & offset)
	{
		MATHVECTOR <T, 3> t0 = offset.cross(normal);
		MATHVECTOR <T, 3> t1 = rotation.GetInvWorldInertia().Multiply(t0);
		MATHVECTOR <T, 3> t2 = t1.cross(offset);
		T inv_mass = linear.GetInvMass() + t2.dot(normal);
		return inv_mass;
	}
	
	// inverse of the effective mass at offset
	MATRIX3 <T> BuildKMatrix(const MATHVECTOR <T, 3> & offset)
	{
		MATRIX3 <T> k;

		float a = offset[0];
		float b = offset[1];
		float c = offset[2];
		
		float m = linear.GetInvMass();
		
		float j0 = rotation.GetInvWorldInertia()[0];
		float j1 = rotation.GetInvWorldInertia()[1];
		float j2 = rotation.GetInvWorldInertia()[2];
		float j3 = rotation.GetInvWorldInertia()[3];
		float j4 = rotation.GetInvWorldInertia()[4];
		float j5 = rotation.GetInvWorldInertia()[5];
		float j6 = rotation.GetInvWorldInertia()[6];
		float j7 = rotation.GetInvWorldInertia()[7];
		float j8 = rotation.GetInvWorldInertia()[8];
		
		k[0] = c * c * j4 - b * c * (j5 + j7) + b * b * j8 + m;
		k[1] = -(c * c * j3) + a * c * j5 + b * c * j6 - a * b * j8;
		k[2] = b * c * j3 - a * c * j4 - b * b * j6 + a * b * j7;
		k[3] = -(c * c * j1) + b * c * j2 + a * c * j7 - a * b * j8;
		k[4] = c * c * j0 - a * c * (j2 + j6) + a * a * j8 + m;
		k[5] = -(b * c * j0) + a * c * j1 + a * b * j6 - a * a * j7;
		k[6] = b * c * j1 - b * b * j2 - a * c * j4 + a * b * j5;
		k[7] = -(b * c * j0) + a * b * j2 + a * c * j3 - a * a * j5;
		k[8] = b * b * j0 - a * b * (j1 + j3) + a * a * j4 + m;
		
		return k;
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,linear);
		_SERIALIZE_(s,rotation);
		return true;
	}
};

#endif
