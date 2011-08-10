#ifndef _CARENGINE_H
#define _CARENGINE_H

#include "driveshaft.h"
#include "LinearMath/btVector3.h"
#include "spline.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>
#include <vector>

class PTree;

struct CARENGINEINFO
{
	btScalar redline; ///< the redline in RPMs
	btScalar rpm_limit; ///< peak engine RPMs after which limiting occurs
	btScalar idle; ///< idle throttle percentage; this is calculated algorithmically
	btScalar start_rpm; ///< initial condition RPM
	btScalar stall_rpm; ///< RPM at which the engine dies
	btScalar fuel_consumption; ///< fuel consumed each second (in liters) is the fuel-consumption parameter times RPM times throttle
	btScalar friction; ///< friction coefficient from the engine; this is calculated algorithmically
	SPLINE<btScalar> torque_curve;
	btVector3 position;
	btScalar inertia;
	btScalar mass;

	/// default constructor makes an S2000-like car
	/// SetTorqueCurve() has to be called explicitly to initialise carengineinfo
	CARENGINEINFO();

	/// load engine from config file
	bool Load(const PTree & cfg, std::ostream & error_output);

	/// Set the torque curve using a vector of (RPM, torque) pairs.
	/// also recalculate engine friction
	void SetTorqueCurve(
		btScalar redline,
		const std::vector<std::pair<btScalar, btScalar> > & torque);

	btScalar GetTorque(
		btScalar throttle,
		btScalar rpm) const;

	btScalar GetFrictionTorque(
		btScalar angvel,
		btScalar friction_factor,
		btScalar throttle_position) const;
};

class CARENGINE
{
friend class joeserialize::Serializer;
public:
	CARENGINE();

	void Init(const CARENGINEINFO & info);

	btScalar GetRPMLimit() const
	{
		return info.rpm_limit;
	}

	btScalar GetRedline() const
	{
		return info.redline;
	}

	btScalar GetIdle() const
	{
		return info.idle;
	}

	btScalar GetStartRPM() const
	{
		return info.start_rpm;
	}

	btScalar GetStallRPM() const
	{
		return info.stall_rpm;
	}

	btScalar GetFuelConsumption() const
	{
		return info.fuel_consumption;
	}

	const btVector3 & GetPosition() const
	{
		return info.position;
	}

	btScalar GetInertia() const
	{
		return info.inertia;
	}

	btScalar GetMass() const
	{
		return info.mass;
	}

	btScalar GetRPM() const
	{
		return shaft.ang_velocity * 30.0 / 3.141593;
	}

	btScalar GetThrottle() const
	{
		return throttle_position;
	}

	btScalar GetAngularVelocity() const
	{
		return shaft.ang_velocity ;
	}

	///return the sum of all torques acting on the engine (except clutch forces)
	btScalar GetTorque() const
	{
		return combustion_torque + friction_torque;
	}

	btScalar FuelRate() const
	{
		return info.fuel_consumption * shaft.ang_velocity * throttle_position;
	}

	///returns true if the engine is combusting fuel
	bool GetCombustion() const
	{
		return !stalled;
	}

	void StartEngine()
	{
		shaft.ang_velocity = info.start_rpm * 3.141593 / 30.0;
	}

	///set the throttle position where 0.0 is no throttle and 1.0 is full throttle
	void SetThrottle(btScalar value)
	{
		throttle_position = value;
	}

	void SetOutOfGas(bool value)
	{
		out_of_gas = value;
	}

	/// calculate torque acting on crankshaft, update engine angular velocity
	btScalar Integrate(btScalar clutch_drag, btScalar clutch_angvel, btScalar dt);

	void DebugPrint(std::ostream & out) const;

	bool Serialize(joeserialize::Serializer & s);

private:
	CARENGINEINFO info;

	DriveShaft shaft;
	btScalar combustion_torque;
	btScalar friction_torque;
	btScalar clutch_torque;

	btScalar throttle_position;
	bool rev_limit_exceeded;
	bool out_of_gas;
	bool stalled;
};

#endif
