#ifndef _CARENGINE_H
#define _CARENGINE_H

#include <iostream>
#include <vector>

#include "rotationalframe.h"
#include "mathvector.h"
#include "spline.h"
#include "joeserialize.h"
#include "macros.h"

template <typename T>
struct CARENGINEINFO
{
	T redline; ///< the redline in RPMs
	T rpm_limit; ///< peak engine RPMs after which limiting occurs
	T idle; ///< idle throttle percentage; this is calculated algorithmically
	T start_rpm; ///< initial condition RPM
	T stall_rpm; ///< RPM at which the engine dies
	T fuel_consumption; ///< fuel consumed each second (in liters) is the fuel-consumption parameter times RPM times throttle
	T friction; ///< friction coefficient from the engine; this is calculated algorithmically
	SPLINE <T> torque_curve;
	MATHVECTOR <T, 3> position;
	T inertia;
	T mass;
	
	/// default constructor makes an S2000-like car
	/// SetTorqueCurve() has to be called explicitly to initialise carengineinfo
	CARENGINEINFO();
	
	/// Set the torque curve using a vector of (RPM, torque) pairs.
	/// also recalculate engine friction
	void SetTorqueCurve(T redline, std::vector < std::pair <T, T> > torque);
	
	T GetTorque(const T throttle, const T rpm) const;
	
	T GetFrictionTorque(T angvel, T friction_factor, T throttle_position);
};

template <typename T>
class CARENGINE
{
friend class joeserialize::Serializer;
public:
	CARENGINE();
	
	void Init(const CARENGINEINFO<T> & info);
	
	T GetRPMLimit() const
	{
		return info.rpm_limit;
	}
	
	T GetRedline() const
	{
		return info.redline;
	}
	
	T GetIdle() const
	{
		return info.idle;
	}
	
	T GetStartRPM() const
	{
		return info.start_rpm;
	}
	
	T GetStallRPM() const
	{
		return info.stall_rpm;
	}
	
	T GetFuelConsumption() const
	{
		return info.fuel_consumption;
	}
	
	MATHVECTOR <T, 3> GetPosition() const
	{
		return info.position;
	}
	
	T GetInertia() const
	{
		return info.inertia;
	}
	
	T GetMass() const
	{
		return info.mass;
	}
	
	const T GetRPM() const
	{
		return crankshaft.GetAngularVelocity()[0] * 30.0 / 3.141593;
	}
	
	T GetThrottle() const
	{
		return throttle_position;
	}
	
	T GetAngularVelocity() const
	{
		return crankshaft.GetAngularVelocity()[0];
	}
	
	///return the sum of all torques acting on the engine (except clutch forces)
	T GetTorque() const
	{
		return combustion_torque + friction_torque;
	}
	
	T FuelRate() const
	{
		return info.fuel_consumption * GetAngularVelocity() * throttle_position;
	}
	
	///returns true if the engine is combusting fuel
	bool GetCombustion() const
	{
		return !stalled;
	}
	
	void SetInitialConditions()
	{
		MATHVECTOR <T, 3> v;
		crankshaft.SetInitialTorque(v);
		StartEngine();
	}
	
	void StartEngine()
	{
		MATHVECTOR <T, 3> v(info.start_rpm * 3.141593 / 30.0, 0, 0);
		crankshaft.SetAngularVelocity(v);
	}
	
	///set the throttle position where 0.0 is no throttle and 1.0 is full throttle
	void SetThrottle(const T& value)
	{
		throttle_position = value;
	}
	
	void SetOutOfGas(bool value)
	{
		out_of_gas = value;
	}
	
	void Integrate1(const T dt)
	{
		crankshaft.Integrate1(dt);
	}
	
	void Integrate2(const T dt)
	{
		crankshaft.Integrate2(dt);
	}
	
	///calculate torque acting on crankshaft
	///to be caled between Integrate1() and Integrate2()
	T ComputeForces(T clutch_drag, T clutch_angvel, T dt);
	
	void DebugPrint(std::ostream & out) const;
	
	bool Serialize(joeserialize::Serializer & s);
	
private:
	CARENGINEINFO <T> info;
	
	ROTATIONALFRAME <T> crankshaft;
	T throttle_position;
	T clutch_torque;
	bool out_of_gas;
	bool rev_limit_exceeded;
	
	//for info only
	T friction_torque;
	T combustion_torque;
	bool stalled;
};

#endif
