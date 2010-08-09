#ifndef _CARWHEEL_H
#define _CARWHEEL_H

#include <iostream>
#include "mathvector.h"
#include "rotationalframe.h"
#include "matrix3.h"
#include "joeserialize.h"
#include "macros.h"

template <typename T>
class CARWHEEL
{
friend class joeserialize::Serializer;
public:
	//default constructor makes an S2000-like car
	CARWHEEL() : mass(18.14)
	{
		SetInertia(10.0);
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Wheel---" << std::endl;
		out << "Wheel speed: " << GetRPM() << std::endl;
	}

	T GetRPM() const
	{
		return rotation.GetAngularVelocity()[1] * 30.0 / 3.141593;
	}

	//used for telemetry only
	const T & GetAngVelInfo()
	{
		return angvel;
	}

	T GetAngularVelocity() const
	{
		return rotation.GetAngularVelocity()[1];
	}

	void SetAngularVelocity(T angvel)
	{
		MATHVECTOR <T, 3> v(0, angvel, 0);
		return rotation.SetAngularVelocity(v);
	}

	void SetMass ( const T& value )
	{
		mass = value;
	}

	T GetMass() const
	{
		return mass;
	}

	void SetInertia(T new_inertia)
	{
		MATRIX3 <T> inertia;
		inertia.Scale(new_inertia);
		rotation.SetInertia(inertia);
	}

	T GetInertia() const
	{
		return rotation.GetInertia()[0];
	}

	void SetInitialConditions()
	{
		MATHVECTOR <T, 3> v;
		rotation.SetInitialTorque(v);
	}

	void Integrate1(const T dt)
	{
		rotation.Integrate1(dt);
	}

	void Integrate2(const T dt)
	{
		rotation.Integrate2(dt);
	}

	void SetTorque(const T torque)
	{
		MATHVECTOR <T, 3> v(0, torque, 0);
		rotation.SetTorque(v);
		angvel = GetAngularVelocity();
	}

	T GetTorque()
	{
		return rotation.GetTorque()[1];
	}

	T GetLockUpTorque(const T dt) const
	{
		const MATHVECTOR<T, 3> w(0);
	    return rotation.GetTorque(w, dt)[1];
	}

	void ZeroForces()
	{
		MATHVECTOR <T, 3> v;
		rotation.SetTorque(v);
	}

	const QUATERNION <T> & GetRotation() const
	{
		return rotation.GetOrientation();
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		T inertia = rotation.GetInertia()[0];
		_SERIALIZE_(s,inertia);
		return true;
	}

private:
	//constants (not actually declared as const because they can be changed after object creation)
	T mass; ///< the mass of the wheel
	ROTATIONALFRAME <T> rotation; ///< a simulation of wheel rotation.  this contains the wheel orientation, angular velocity, angular acceleration, and inertia tensor

	//for info only
	T angvel;
};

#endif
