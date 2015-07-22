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

#ifndef _CARDYNAMICS_H
#define _CARDYNAMICS_H

#include "carengine.h"
#include "carclutch.h"
#include "cartransmission.h"
#include "cardifferential.h"
#include "carfueltank.h"
#include "carsuspension.h"
#include "carwheel.h"
#include "cartire.h"
#include "carbrake.h"
#include "carwheelposition.h"
#include "aerodevice.h"
#include "collision_contact.h"
#include "motionstate.h"
#include "joeserialize.h"

#include "BulletDynamics/Dynamics/btActionInterface.h"

struct btCollisionObjectWrapper;
class btCollisionWorld;
class btManifoldPoint;
class btIDebugDraw;
class DynamicsWorld;
class FractureBody;
class ContentManager;
class PTree;

class CarDynamics : public btActionInterface
{
friend class joeserialize::Serializer;

public:
	CarDynamics();

	CarDynamics(const CarDynamics & other);

	CarDynamics & operator= (const CarDynamics & other);

	~CarDynamics();

	// tirealt is optional tire config, overrides default tire type
	bool Load(
		const PTree & cfg,
		const std::string & cardir,
		const std::string & cartire,
		const btVector3 & position,
		const btQuaternion & rotation,
		const bool damage,
		DynamicsWorld & world,
		ContentManager & content,
		std::ostream & error);

	// set body position
	void SetPosition(const btVector3 & pos);

	// move the car along z-axis until it is touching the ground
	void AlignWithGround();

	// fime: move into car input vector?
	void SetAutoClutch(bool value);
	void SetAutoShift(bool value);
	void SetABS(bool value);
	void SetTCS(bool value);

	// update dynamics from car input vector
	void Update(const std::vector<float> & inputs);

	// bullet interface
	void updateAction(btCollisionWorld * collisionWorld, btScalar dt);
	void debugDraw(btIDebugDraw * debugDrawer);

	// graphics interpolated
	btVector3 GetEnginePosition() const;
	const btVector3 & GetPosition() const;
	const btQuaternion & GetOrientation() const;
	const btVector3 & GetWheelPosition(WheelPosition wp) const;
	const btQuaternion & GetWheelOrientation(WheelPosition wp) const;
	btQuaternion GetUprightOrientation(WheelPosition wp) const;

	// intepolated bodies transform: body, wheels, ...
	unsigned GetNumBodies() const;
	const btVector3 & GetPosition(int i) const;
	const btQuaternion & GetOrientation(int i) const;

	// collision world interface
	const CollisionContact & GetWheelContact(WheelPosition wp) const;
	CollisionContact & GetWheelContact(WheelPosition wp);

	// body
	const btVector3 & GetWheelVelocity(WheelPosition wp) const;
	const btVector3 & GetCenterOfMass() const;
	const btVector3 & GetVelocity() const;
	btScalar GetInvMass() const;
	btScalar GetSpeed() const;

	// speedometer reading front left wheel in m/s
	btScalar GetSpeedMPS() const;

	// based on transmission, engine rpm limit and wheel radius in m/s
	btScalar GetMaxSpeedMPS() const;

	// engine rpm
	btScalar GetTachoRPM() const;

	// driveline state access
	const CarEngine & GetEngine() const {return engine;}
	const CarClutch & GetClutch() const {return clutch;}
	const CarTransmission & GetTransmission() const {return transmission;}
	const CarBrake & GetBrake(WheelPosition pos) const {return brake[pos];}
	const CarWheel & GetWheel(WheelPosition pos) const {return wheel[pos];}
	const CarTire & GetTire(WheelPosition pos) const {return tire[pos];}
	btScalar GetFuelAmount() const {return fuel_tank.FuelPercent();}
	btScalar GetNosAmount() const {return engine.GetNosAmount();}
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;

	// get the maximum steering angle in degrees
	btScalar GetMaxSteeringAngle() const;

	const CarSuspension & GetSuspension(WheelPosition pos) const {return *suspension[pos];}

	const btVector3 & GetCenterOfMassOffset() const;

	btScalar GetAerodynamicDownforceCoefficient() const;

	btScalar GetAeordynamicDragCoefficient() const;

	btVector3 GetTotalAero() const;

	btScalar GetFeedback() const;

	btScalar GetTireSquealAmount(WheelPosition i) const;

	// This is needed for ray casts in the AI implementation.
	DynamicsWorld * getDynamicsWorld() const {return world;}

	const btCollisionObject & getCollisionObject() const;

	btVector3 LocalToWorld(const btVector3 & local) const;

	// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	bool Serialize(joeserialize::Serializer & s);

	static bool WheelContactCallback(
		btManifoldPoint& cp,
		const btCollisionObjectWrapper* col0,
		int partId0,
		int index0,
		const btCollisionObjectWrapper* col1,
		int partId1,
		int index1);

protected:
	DynamicsWorld * world;
	FractureBody * body;

	// body state
	btTransform transform;
	btVector3 linear_velocity;
	btVector3 angular_velocity;
	btAlignedObjectArray<MotionState> motion_state;

	// driveline state
	CarEngine engine;
	CarFuelTank fuel_tank;
	CarClutch clutch;
	CarTransmission transmission;
	CarDifferential differential_front;
	CarDifferential differential_rear;
	CarDifferential differential_center;
	btAlignedObjectArray<CarBrake> brake;
	btAlignedObjectArray<CarWheel> wheel;
	btAlignedObjectArray<CarTire> tire;
	btAlignedObjectArray<CarSuspension*> suspension;
	btAlignedObjectArray<AeroDevice> aerodevice;

	// wheel contact state
	btAlignedObjectArray<CollisionContact> wheel_contact;
	btAlignedObjectArray<btVector3> suspension_force;
	btAlignedObjectArray<btVector3> wheel_velocity;
	btAlignedObjectArray<btVector3> wheel_position;
	btAlignedObjectArray<btQuaternion> wheel_orientation;

	enum { NONE = 0, FWD = 1, RWD = 2, AWD = 3 } drive;
	btScalar driveshaft_rpm;
	btScalar tacho_rpm;

	bool autoclutch;
	bool autoshift;
	bool shifted;
	int shift_gear;
	btScalar remaining_shift_time;
	btScalar clutch_value;
	btScalar brake_value;

	// traction control state
	bool abs;
	bool tcs;
	std::vector<int> abs_active;
	std::vector<int> tcs_active;

	btScalar maxangle;
	btScalar maxspeed;
	btScalar feedback_scale;
	btScalar feedback;

	btVector3 GetDownVector() const;

	btQuaternion LocalToWorld(const btQuaternion & local) const;

	void UpdateWheelVelocity();

	void UpdateWheelTransform();

	void ApplyEngineTorqueToBody(btVector3 & torque);

	void ApplyAerodynamicsToBody(btVector3 & force, btVector3 & torque);

	void ComputeSuspensionDisplacement(int i, btScalar dt);

	void DoTCS(int i, btScalar suspension_force);

	void DoABS(int i, btScalar suspension_force);

	btVector3 ApplySuspensionForceToBody(int i, btScalar dt, btVector3 & force, btVector3 & torque);

	btVector3 ComputeTireFrictionForce(int i, btScalar dt, btScalar normal_force,
		btScalar rotvel, const btVector3 & linvel, const btQuaternion & wheel_orientation);

	void ApplyWheelForces(btScalar dt, btScalar wheel_drive_torque, int i, const btVector3 & suspension_force, btVector3 & force, btVector3 & torque);

	void ApplyForces(btScalar dt, const btVector3 & force, const btVector3 & torque);

	void Tick(btScalar dt, const btVector3 & force, const btVector3 & torque);

	void UpdateWheelContacts();

	void InterpolateWheelContacts();

	// update engine, return wheel drive torque
	void UpdateDriveline(btScalar drive_torque[], btScalar dt);

	// calculate wheel drive torque
	void CalculateDriveTorque(btScalar drive_torque[], btScalar clutch_torque);

	// calculate driveshaft speed given wheel angular velocity
	btScalar CalculateDriveshaftSpeed();

	// calculate throttle, clutch, gear
	void UpdateTransmission(btScalar dt);

	bool WheelDriven(int i) const;

	btScalar AutoClutch(btScalar dt) const;

	btScalar ShiftAutoClutchThrottle(btScalar throttle, btScalar dt);

	// calculate next gear based on engine rpm
	int NextGear() const;

	// calculate downshift point based on gear, engine rpm
	btScalar DownshiftRPM(int gear) const;

	// max speed in m/s calculated from maxrpm, maxgear, finalgear ratios
	btScalar CalculateMaxSpeed() const;

	// car controls

	void SetSteering(const btScalar value);

	void StartEngine();

	void ShiftGear(int value);

	void SetThrottle(btScalar value);

	void SetNOS(btScalar value);

	void SetClutch(btScalar value);

	void SetBrake(btScalar value);

	void SetHandBrake(btScalar value);

	void RolloverRecover();

	void Clear();

	void Init();
};

#endif
