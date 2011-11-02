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
	btScalar redline; ///< the redline in RPMs (used only for the redline graphics)
	btScalar rpm_limit; ///< peak engine RPMs after which limiting occurs
	btScalar idle; ///< idle throttle percentage; this is calculated algorithmically
	btScalar start_rpm; ///< initial condition RPM
	btScalar stall_rpm; ///< RPM at which the engine dies
	btScalar fuel_rate; ///< fuel rate kg/Ws based on fuel heating value(4E7) and engine efficiency(0.35)
	btScalar friction; ///< friction coefficient from the engine; this is calculated algorithmically
	SPLINE<btScalar> torque_curve;
	btVector3 position;
	btScalar inertia;
	btScalar mass;
	btScalar nos_mass; ///< amount of nitrous oxide in kg
	btScalar nos_boost; ///< max nitrous oxide power boost in Watt
	btScalar nos_fuel_ratio; ///< nos to fuel ratio(5)

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
		return shaft.ang_velocity * 30.0 / M_PI;
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

	///fuel consumtion rate kg/s per simulation step
	btScalar FuelRate() const
	{
		return info.fuel_rate * combustion_torque * shaft.ang_velocity;
	}

	///returns true if the engine is combusting fuel
	bool GetCombustion() const
	{
		return !stalled;
	}

	///return remaining nos fraction
	btScalar GetNosAmount() const
	{
		return nos_mass / info.nos_mass;
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

	/// nitrous injection boost factor 0.0 - 1.0
	void SetNosBoost(btScalar value)
	{
		nos_boost_factor = value;
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
	btScalar nos_boost_factor;
	btScalar nos_mass;
	bool rev_limit_exceeded;
	bool out_of_gas;
	bool stalled;
};

#endif
