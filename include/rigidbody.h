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
	
	//access to rotational frame
	void SetInitialTorque(const MATHVECTOR <T, 3> & torque) {rotation.SetInitialTorque(torque);}
	void SetTorque(const MATHVECTOR <T, 3> & torque) {rotation.SetTorque(torque);}
	void SetInertia(const MATRIX3 <T> & inertia) {rotation.SetInertia(inertia);}
	const MATRIX3 <T> & GetInertia() {return rotation.GetInertia();}
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
	
	/// given the input force (in world space) applied at the given offset from the center of mass (in world space), add the force and torque generated at the center of mass to the output vectors (in world space)
	void GetForceAtOffset(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset, MATHVECTOR <T, 3> & output_force, MATHVECTOR <T, 3> & output_torque)
	{
		//here's the equation:
		//F_linear = F
		//F_torque = F cross (distance from center of mass)
		
		output_force = output_force + force;
		MATHVECTOR <T, 3> zero;
		MATHVECTOR <T, 3> torque = force.cross(zero-offset);
		output_torque = output_torque + torque;
		
		/*MATHVECTOR <T, 3> body_force = force;
		(-GetOrientation()).RotateVector(body_force);
		MATHVECTOR <T, 3> body_offset = offset;
		(-GetOrientation()).RotateVector(body_offset);
		MATHVECTOR <T, 3> body_torque = torque;
		(-GetOrientation()).RotateVector(body_torque);
		std::cout << "force = " << body_force << ", offset = " << body_offset << ", torque = " << body_torque << std::endl;*/
	}
	
	void GetTorqueAtOffset(const MATHVECTOR <T, 3> & torque, const MATHVECTOR <T, 3> & offset, MATHVECTOR <T, 3> & output_torque)
	{
	    //here's the equation:
	    //T_torque = T * (I/(I+mass*D.D))
        if (torque.Magnitude() > 1e-9)
		{
			MATRIX3 <T> inertia_tensor = rotation.GetInertia();
			MATHVECTOR <T, 3> bodytorque = torque;
			(-GetOrientation()).RotateVector(bodytorque);
			T inertia_factor = (inertia_tensor.Multiply(bodytorque.Normalize())).Magnitude();
			MATHVECTOR <T, 3> newtorque = bodytorque * (inertia_factor/(inertia_factor + linear.GetMass() * (offset.dot(offset))));
            //std::cout << bodytorque << " -- " << newtorque << std::endl;
			GetOrientation().RotateVector(newtorque);
			output_torque = output_torque + newtorque;
            //output_torque = output_torque - newtorque;
		}
	}
	
	/// given the input force (in world space) and torque (in world space) applied at the given offset from the center of mass (in world space), add the force and torque generated at the center of mass to the output vectors (in world space)
	void GetForceAndTorqueAtOffset(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & torque, const MATHVECTOR <T, 3> & offset, MATHVECTOR <T, 3> & output_force, MATHVECTOR <T, 3> & output_torque)
	{
		GetForceAtOffset(force, offset, output_force, output_torque);
		GetTorqueAtOffset(torque, offset, output_torque);
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,linear);
		_SERIALIZE_(s,rotation);
		return true;
	}
};

#endif
