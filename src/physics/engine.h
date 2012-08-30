/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _SIM_ENGINE_H
#define _SIM_ENGINE_H

#include "shaft.h"
#include "LinearMath/btVector3.h"
#include <vector>

namespace sim
{

struct EngineInfo
{
	std::vector<btScalar> torque; ///< n entries, rpm delta (rpm_limit - rpm_stall) / n
	btScalar redline;			///< the redline in RPMs (used only for the redline graphics)
	btScalar rpm_limit;			///< peak engine RPMs after which limiting occurs
	btScalar rpm_start;			///< initial condition RPM
	btScalar rpm_stall;			///< RPM at which the engine dies
	btScalar idle;				///< idle throttle percentage; this is calculated algorithmically
	btScalar fuel_rate;			///< fuel rate kg/Ws based on fuel heating value(4E7) and engine efficiency(0.35)
	btScalar friction;			///< friction coefficient from the engine; this is calculated algorithmically
	btVector3 position;			///< engine position
	btScalar inertia;			///< engine shaft rotational inertia
	btScalar mass;				///< engine mass in kg
	btScalar fuel_mass;			///< amount of fuel available in kg
	btScalar fuel_capacity;		///< max fuel capacity in kg
	btScalar nos_mass;			///< amount of nitrous oxide in kg
	btScalar nos_boost;			///< max nitrous oxide power boost in Watt
	btScalar nos_fuel_ratio;	///< nos to fuel ratio(5)
	EngineInfo();				///< default constructor makes an S2000-like car
};

class Engine
{
public:
	/// default constructor (use init)
	Engine();

	/// setup engine
	void init(const EngineInfo & info);

	/// start engine by setting crankshaft rpm to start rpm
	void start();

	/// update combustion and friction torques, consume fuel, nos
	void update(btScalar dt);

	/// set the throttle position where 0.0 is no throttle and 1.0 is full throttle
	void setThrottle(btScalar value);

	/// set nitrous injection boost factor 0.0 - 1.0
	void setNosBoost(btScalar value);

	/// maximum possible rpm
	btScalar getRPMLimit() const;

	/// maximum allowed rpm
	btScalar getRedline() const;

	/// throttle position to keep engine at idle rpm
	btScalar getIdleThrottle() const;

	/// engine stall rpm
	btScalar getStallRPM() const;

	/// rpm at engine start
	btScalar getStartRPM() const;

	/// relative engine position
	const btVector3 & getPosition() const;

	/// engine shaft inertia
	btScalar getInertia() const;

	/// engine mass in kg
	btScalar getMass() const;

	/// current rpm
	btScalar getRPM() const;

	/// current throttle position
	btScalar getThrottle() const;

	/// current angular velocity rad/s
	btScalar getAngularVelocity() const;

	/// current shaft torgue in Nm
	btScalar getTorque() const;

	/// true if the engine is combusting fuel
	bool getCombustion() const;

	/// available fuel fraction
	btScalar getFuel() const;

	/// available nos fraction
	btScalar getNos() const;

	/// get engine crankshaft
	const Shaft & getShaft() const;
	Shaft & getShaft();

private:
	EngineInfo info;
	Shaft shaft;
	btScalar combustion_torque;
	btScalar friction_torque;
	btScalar throttle_position;
	btScalar nos_boost_factor;
	btScalar fuel_mass;
	btScalar nos_mass;
	bool rev_limit_exceeded;
	bool stalled;

	btScalar getCombustionTorque(
		btScalar throttle,
		btScalar angvel) const;

	btScalar getFrictionTorque(
		btScalar throttle,
		btScalar angvel) const;

	btScalar calcIdleThrottle() const;

	btScalar calcEngineFriction() const;
};

// implementation

inline void Engine::setThrottle(btScalar value)
{
	throttle_position = value;
}

inline void Engine::setNosBoost(btScalar value)
{
	nos_boost_factor = value;
}

inline btScalar Engine::getRPMLimit() const
{
	return info.rpm_limit;
}

inline btScalar Engine::getRedline() const
{
	return info.redline;
}

inline btScalar Engine::getIdleThrottle() const
{
	return info.idle;
}

inline btScalar Engine::getStallRPM() const
{
	return info.rpm_stall;
}

inline btScalar Engine::getStartRPM() const
{
	return info.rpm_start;
}

inline const btVector3 & Engine::getPosition() const
{
	return info.position;
}

inline btScalar Engine::getInertia() const
{
	return info.inertia;
}

inline btScalar Engine::getMass() const
{
	return info.mass;
}

inline btScalar Engine::getRPM() const
{
	return shaft.getAngularVelocity() * 30.0 / M_PI;
}

inline btScalar Engine::getThrottle() const
{
	return throttle_position;
}

inline btScalar Engine::getAngularVelocity() const
{
	return shaft.getAngularVelocity();
}

inline btScalar Engine::getTorque() const
{
	return combustion_torque + friction_torque;
}

inline bool Engine::getCombustion() const
{
	return !stalled;
}

inline btScalar Engine::getFuel() const
{
	return fuel_mass / info.fuel_capacity;
}

inline btScalar Engine::getNos() const
{
	return nos_mass / info.nos_mass;
}

inline const Shaft & Engine::getShaft() const
{
	return shaft;
}

inline Shaft & Engine::getShaft()
{
	return shaft;
}

}

#endif
