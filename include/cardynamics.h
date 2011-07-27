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
#include "caraerodynamicdevice.h"
#include "collision_contact.h"
#include "cartelemetry.h"
#include "motionstate.h"
#include "joeserialize.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "LinearMath/btAlignedObjectArray.h"

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

class DynamicsWorld;
class PTree;

class CARDYNAMICS : public btActionInterface
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	CARDYNAMICS();

	~CARDYNAMICS();

	bool Load(
		const PTree & cfg,
		const btVector3 & meshsize,
		const btVector3 & meshcenter,
		const btVector3 & position,
		const btQuaternion & rotation,
		const bool damage,
		DynamicsWorld & world,
		std::ostream & error_output);

	// bullet interface
	void updateAction(btCollisionWorld * collisionWorld, btScalar dt);
	void debugDraw(btIDebugDraw * debugDrawer);

	// graphics interpolated
	void Update();
	btVector3 GetEnginePosition() const;
	const btVector3 & GetPosition() const;
	const btQuaternion & GetOrientation() const;
	const btVector3 & GetWheelPosition(WHEEL_POSITION wp) const;
	const btQuaternion & GetWheelOrientation(WHEEL_POSITION wp) const;
	btQuaternion GetUprightOrientation(WHEEL_POSITION wp) const;

	// intepolated bodies transform: body, wheels, ...
	unsigned GetNumBodies() const;
	const btVector3 & GetPosition(int i) const;
	const btQuaternion & GetOrientation(int i) const;
	const btCollisionShape * GetShape() const;

	// collision world interface
	const COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp) const;
	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp);

	// body
	const btVector3 & GetWheelVelocity(WHEEL_POSITION wp) const;
	const btVector3 & GetCenterOfMass() const;
	const btVector3 & GetVelocity() const;
	btScalar GetInvMass() const;
	btScalar GetSpeed() const;

	// driveline control
	void StartEngine();
	void ShiftGear(int value);
	void SetThrottle(btScalar value);
	void SetClutch(btScalar value);
	void SetBrake(btScalar value);
	void SetHandBrake(btScalar value);
	void SetAutoClutch(bool value);
	void SetAutoShift(bool value);

	// first wheel velocity
	btScalar GetSpeedMPS() const;

	// engine rpm
	btScalar GetTachoRPM() const;

	// driveline state access
	const CARENGINE & GetEngine() const {return engine;}
	const CARCLUTCH & GetClutch() const {return clutch;}
	const CARTRANSMISSION & GetTransmission() const {return transmission;}
	const CARBRAKE & GetBrake(WHEEL_POSITION pos) const {return brake[pos];}
	const CARWHEEL & GetWheel(WHEEL_POSITION pos) const {return wheel[pos];}
	const CARTIRE & GetTire(WHEEL_POSITION pos) const {return tire[pos];}

	// traction control
	void SetABS(const bool newabs);
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	void SetTCS(const bool newtcs);
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;

	// set body position
	void SetPosition(const btVector3 & pos);

	// move the car along z-axis until it is touching the ground
	void AlignWithGround();

	// rotate car back onto it's wheels after rollover
	void RolloverRecover();

	// set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
	void SetSteering(const btScalar value);

	// get the maximum steering angle in degrees
	btScalar GetMaxSteeringAngle() const;

	const CARSUSPENSION & GetSuspension(WHEEL_POSITION pos) const {return *suspension[pos];}

	btScalar GetAerodynamicDownforceCoefficient() const;

	btScalar GetAeordynamicDragCoefficient() const;

	btVector3 GetTotalAero() const;

	btScalar GetFeedback() const;

	void UpdateTelemetry(btScalar dt);

	// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	bool Serialize(joeserialize::Serializer & s);

protected:
	DynamicsWorld* world;
	btRigidBody* body;

	// body state
	btTransform transform;
	btVector3 linear_velocity;
	btVector3 angular_velocity;
	btAlignedObjectArray<MotionState> motion_state;

	// driveline state
	CARENGINE engine;
	CARFUELTANK fuel_tank;
	CARCLUTCH clutch;
	CARTRANSMISSION transmission;
	CARDIFFERENTIAL differential_front;
	CARDIFFERENTIAL differential_rear;
	CARDIFFERENTIAL differential_center;
	btAlignedObjectArray<CARBRAKE> brake;
	btAlignedObjectArray<CARWHEEL> wheel;
	btAlignedObjectArray<CARTIRE> tire;

	enum { NONE = 0, FWD = 1, RWD = 2, AWD = 3 } drive;
	btScalar driveshaft_rpm;
	btScalar tacho_rpm;

	bool autoclutch;
	bool autoshift;
	bool shifted;
	int shift_gear;
	btScalar last_auto_clutch;
	btScalar remaining_shift_time;

	// traction control state
	bool abs;
	bool tcs;
	std::vector<int> abs_active;
	std::vector<int> tcs_active;

	// suspension
	btAlignedObjectArray<btVector3> suspension_force;
	btAlignedObjectArray<btVector3> wheel_velocity;
	btAlignedObjectArray<btVector3> wheel_position;
	btAlignedObjectArray<btQuaternion> wheel_orientation;
	btAlignedObjectArray<COLLISION_CONTACT> wheel_contact;
	std::vector<std::tr1::shared_ptr<CARSUSPENSION> > suspension;

	btAlignedObjectArray<CARAERO> aerodynamics;
	std::list<CARTELEMETRY> telemetry;

	btScalar maxangle;
	btScalar feedback;
	bool damage;

	btVector3 GetDownVector() const;

	const btVector3 & GetCenterOfMassOffset() const;

	btVector3 LocalToWorld(const btVector3 & local) const;

	btQuaternion LocalToWorld(const btQuaternion & local) const;

	void UpdateWheelVelocity();

	void UpdateWheelTransform();

	void ApplyEngineTorqueToBody ( btVector3 & force, btVector3 & torque );

	void ApplyAerodynamicsToBody ( btVector3 & force, btVector3 & torque );

	void ComputeSuspensionDisplacement ( int i, btScalar dt );

	void DoTCS ( int i, btScalar suspension_force );

	void DoABS ( int i, btScalar suspension_force );

	btVector3 ApplySuspensionForceToBody ( int i, btScalar dt, btVector3 & force, btVector3 & torque );

	btVector3 ComputeTireFrictionForce ( int i, btScalar dt, btScalar normal_force,
        btScalar angvel, btVector3 & groundvel, const btQuaternion & wheel_orientation );

	void ApplyWheelForces ( btScalar dt, btScalar wheel_drive_torque, int i, const btVector3 & suspension_force, btVector3 & force, btVector3 & torque );

	void ApplyForces ( btScalar dt, const btVector3 & force, const btVector3 & torque);

	void Tick ( btScalar dt, const btVector3 & force, const btVector3 & torque);

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

	btScalar AutoClutch(btScalar last_clutch, btScalar dt) const;

	btScalar ShiftAutoClutch() const;

	btScalar ShiftAutoClutchThrottle(btScalar throttle, btScalar dt);

	// calculate next gear based on engine rpm
	int NextGear() const;

	// calculate downshift point based on gear, engine rpm
	btScalar DownshiftRPM(int gear) const;

	// cardynamics initialization
	void GetCollisionBox(
		const btVector3 & bodySize,
		const btVector3 & bodyCenter,
		btVector3 & center,
		btVector3 & size);
};

#endif
