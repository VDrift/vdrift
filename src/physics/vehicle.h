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

#ifndef _SIM_VEHICLE_H
#define _SIM_VEHICLE_H

#include "aerodevice.h"
#include "antirollbar.h"
#include "differential.h"
#include "wheel.h"
#include "transmission.h"
#include "clutch.h"
#include "engine.h"
#include "wheelcontact.h"
#include "differentialjoint.h"
#include "clutchjoint.h"
#include "motorjoint.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "LinearMath/btAlignedObjectArray.h"
#include <ostream>

namespace sim
{

class World;
class FractureBody;
struct VehicleInfo;
struct VehicleInput;
struct VehicleState;

class Vehicle : public btActionInterface
{
public:
	Vehicle();

	~Vehicle();

	/// vehicle info has to be valid, no error checking here!
	void init(
		const VehicleInfo & info,
		const btVector3 & position,
		const btQuaternion & rotation,
		World & world);

	/// vehicle controls input, call before sim step(updateAction)
	void setInput(const VehicleInput & input);

	/// set current vehicle state
	void setState(const VehicleState & state);

	/// get current vehicle state
	void getState(VehicleState & state) const;

	/// executed as last function(after integration) in bullet singlestepsimulation
	void updateAction(btCollisionWorld * collisionWorld, btScalar dt);

	/// unused
	void debugDraw(btIDebugDraw * debugDrawer);

	/// body state
	const btTransform & getTransform() const;
	const btVector3 & getPosition() const;
	const btVector3 & getVelocity() const;
	btScalar getInvMass() const;
	const btCollisionObject * getCollisionObject() const;
	const btCollisionWorld * getCollisionWorld() const;

	/// linear velocity magnitude
	btScalar getSpeed() const;

	/// speedometer reading first wheel(front left) in m/s
	btScalar getSpeedMPS() const;

	/// based on transmission, engine rpm limit and wheel radius in m/s
	btScalar getMaxSpeedMPS() const;

	/// engine rpm
	btScalar getTachoRPM() const;

	/// get wheel steering torque
	btScalar getSteeringTorque() const;

	/// get the maximum steering angle in degrees
	btScalar getMaxSteeringAngle() const;

	/// minimum distance to reach target speed
	btScalar getBrakingDistance(btScalar target_speed) const;

	/// maximum velocity for given curve radius at current speed
	btScalar getMaxVelocity(btScalar radius) const;

	/// caculate aerodynamic force in world space
	btVector3 getTotalAero() const;

	/// get vehicle width (used by ai)
	btScalar getWidth() const;

	/// driveline state access
	int getWheelCount() const;
	const Wheel & getWheel(int i) const;
	const WheelContact & getWheelContact(int i) const;
	const Transmission & getTransmission() const;
	const Clutch & getClutch() const;
	const Engine & getEngine() const;
	btScalar getNosAmount() const;
	btScalar getFuelAmount() const;

	/// traction control state
	bool getABSActive() const;
	bool getTCSActive() const;

	/// debugging
	void print(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	/// to be registered before adding vehicles to world
	static bool WheelContactCallback(
		btManifoldPoint& cp,
		const btCollisionObject* colObj0,
		int partId0,
		int index0,
		const btCollisionObject* colObj1,
		int partId1,
		int index1);

protected:
	World * world;
	FractureBody * body;
	btTransform transform;
	btAlignedObjectArray<AeroDevice> aero_device;
	btAlignedObjectArray<Differential> differential;
	btAlignedObjectArray<AntiRollBar> antiroll;
	btAlignedObjectArray<Wheel> wheel;
	Transmission transmission;
	Clutch clutch;
	Engine engine;

	/// vehicle solver state
	btAlignedObjectArray<WheelContact> wheel_contact;
	btAlignedObjectArray<DifferentialJoint> diff_joint;
	btAlignedObjectArray<ClutchJoint> clutch_joint;
	btAlignedObjectArray<MotorJoint> motor_joint;

	/// vehicle logic state
	btScalar brake_value;
	btScalar last_clutch;
	btScalar remaining_shift_time;
	btScalar tacho_rpm;
	int shift_gear;
	bool autoclutch;
	bool autoshift;
	bool shifted;
	bool abs_active;
	bool tcs_active;
	//bool abs_enabled;
	//bool tcs_enabled;

	/// aerodynamic force and torque for debugging
	btVector3 aero_force;
	btVector3 aero_torque;

	/// approximate total lateral and longitudinal friction coefficients
	/// used for braking distance and max curve velocity estimation
	btScalar lon_friction_coeff;
	btScalar lat_friction_coeff;

	/// wheel steering torque
	btScalar steering_torque;

	/// vehicle constants
	btScalar maxangle;
	btScalar maxspeed;
	btScalar width;

	/// get body down direction
	btVector3 getDownVector() const;

	/// set body position
	void setPosition(const btVector3 & pos);

	/// move the car along z-axis until it is touching the ground
	void alignWithGround();

	/// rotate car back onto it's wheels
	void rolloverRecover();

	/// start vehicle engine (force rpm to start_rpm)
	void startEngine();

	/// set car steering [-1, 1] left, right
	void setSteering(btScalar value);

	/// select gear [nrev, nfwd]
	void setGear(int value);

	/// set engine throttle [0, 1]
	void setThrottle(btScalar value);

	/// set N2O injection value [0, 1]
	void setNOS(btScalar value);

	/// set clutch [0, 1]
	void setClutch(btScalar value);

	/// set brakes [0, 1]
	void setBrake(btScalar value);

	/// set handbrake value [0, 1]
	void setHandBrake(btScalar value);

	/// use auto clutch logic
	void setAutoClutch(bool value);

	/// use auto shift logic
	void setAutoShift(bool value);

	/// enable ABS
	void setABS(bool value);

	/// enable TCS
	void setTCS(bool value);

	/// update driveline, chassis
	void updateDynamics(btScalar dt);

	/// calculate throttle, clutch, gear
	void updateTransmission(btScalar dt);

	/// apply aerodynamic forces to body
	void updateAerodynamics(btScalar dt);

	/// update wheel position, rotation
	void updateWheelTransform(btScalar dt);

	/// calulate new clutch value
	btScalar autoClutch(btScalar clutch_rpm, btScalar last_clutch, btScalar dt) const;

	/// override throttle value
	btScalar autoClutchThrottle(btScalar clutch_rpm, btScalar throttle, btScalar dt);

	/// return the gear change (0 for no change, -1 for shift down, 1 for shift up)
	int getNextGear(btScalar clutch_rpm) const;

	/// calculate downshift rpm based on gear, engine rpm
	btScalar getDownshiftRPM(int gear) const;

	/// max speed in m/s calculated from maxrpm, maxgear, finalgear ratios
	btScalar calculateMaxSpeed() const;

	/// calculate total longitudinal and lateral friction coefficients
	void calculateFrictionCoefficient(btScalar & lon_mu, btScalar & lat_mu) const;

	/// total aerodynamic lift coefficient
	btScalar getLiftCoefficient() const;

	/// total aerodynamic drag coefficient
	btScalar getDragCoefficient() const;
};

// implementation

inline btScalar Vehicle::getMaxSpeedMPS() const
{
	return maxspeed;
}

inline btScalar Vehicle::getTachoRPM() const
{
	return tacho_rpm;
}

inline btScalar Vehicle::getSteeringTorque() const
{
	return steering_torque;
}

inline btScalar Vehicle::getMaxSteeringAngle() const
{
	return maxangle;
}

inline btScalar Vehicle::getWidth() const
{
	return width;
}

inline int Vehicle::getWheelCount() const
{
	return wheel.size();
}

inline const Wheel & Vehicle::getWheel(int i) const
{
	return wheel[i];
}

inline const WheelContact & Vehicle::getWheelContact(int i) const
{
	return wheel_contact[i];
}

inline const Transmission & Vehicle::getTransmission() const
{
	return transmission;
}

inline const Clutch & Vehicle::getClutch() const
{
	return clutch;
}

inline const Engine & Vehicle::getEngine() const
{
	return engine;
}

inline btScalar Vehicle::getNosAmount() const
{
	return engine.getNosAmount();
}

inline float Vehicle::getFuelAmount() const
{
	return engine.getFuelAmount();
}

inline bool Vehicle::getABSActive() const
{
	return abs_active;
}

inline bool Vehicle::getTCSActive() const
{
	return tcs_active;
}

}

#endif
