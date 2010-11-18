#ifndef _CARDYNAMICS_H
#define _CARDYNAMICS_H

#include "rigidbody.h"
#include "carengine.h"
#include "carclutch.h"
#include "cartransmission.h"
#include "cardifferential.h"
#include "carfueltank.h"
#include "carsuspension.h"
#include "carwheel.h"
#include "cartire.h"
#include "carwheelposition.h"
#include "carbrake.h"
#include "caraerodynamicdevice.h"
#include "joeserialize.h"
#include "macros.h"
#include "collision_contact.h"
#include "cartelemetry.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"

class MODEL;
class CONFIG;
class COLLISION_WORLD;
//class SuspensionConstraint;

class CARDYNAMICS : public btActionInterface
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	typedef double T;
	
	CARDYNAMICS();
	
	bool Load(const CONFIG & c, std::ostream & error_output);

	void Init(
		COLLISION_WORLD & world,
		MATHVECTOR <T, 3> chassisSize,
		MATHVECTOR <T, 3> chassisCenter,
		MATHVECTOR <T, 3> position,
		QUATERNION <T> orientation);

// bullet interface
	virtual void updateAction(btCollisionWorld * collisionWorld, btScalar dt);
	virtual void debugDraw(btIDebugDraw * debugDrawer);

// graphics interface, interpolated!
	void Update(); // update interpolated chassis state
	const MATHVECTOR <T, 3> & GetCenterOfMassPosition() const;
	const MATHVECTOR <T, 3> & GetPosition() const;
	const QUATERNION <T> & GetOrientation() const;
	MATHVECTOR <T, 3> GetWheelPosition(WHEEL_POSITION wp) const;
	MATHVECTOR <T, 3> GetWheelPosition(WHEEL_POSITION wp, T displacement_percent) const; // for debugging
	QUATERNION <T> GetWheelOrientation(WHEEL_POSITION wp) const;
	QUATERNION <T> GetUprightOrientation(WHEEL_POSITION wp) const;
	MATHVECTOR <T, 3> GetWheelVelocity(WHEEL_POSITION wp) const;

// collision world interface
	const COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp) const;
	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp);

// chassis
	float GetMass() const;
	T GetSpeed() const;
	MATHVECTOR <T, 3> GetVelocity() const;
	MATHVECTOR <T, 3> GetEnginePosition() const;

// driveline
	// driveline input
	void StartEngine();
	void ShiftGear(int value);
	void SetThrottle(float value);
	void SetClutch(float value);
	void SetBrake(float value);
	void SetHandBrake(float value);
	void SetAutoClutch(bool value);
	void SetAutoShift(bool value);

	// first wheel velocity
	T GetSpeedMPS() const;
	
	// engine rpm
	T GetTachoRPM() const;

	// driveline state access
	const CARENGINE <T> & GetEngine() const {return engine;}
	const CARCLUTCH <T> & GetClutch() const {return clutch;}
	const CARTRANSMISSION <T> & GetTransmission() const {return transmission;}
	const CARBRAKE <T> & GetBrake(WHEEL_POSITION pos) const {return brake[pos];}
	const CARWHEEL <T> & GetWheel(WHEEL_POSITION pos) const {return wheel[pos];}

// traction control
	void SetABS(const bool newabs);
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	void SetTCS(const bool newtcs);
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;

// cardynamics
	void SetPosition(const MATHVECTOR<T, 3> & pos);

	// move the car along z-axis until it is touching the ground
	void AlignWithGround();
	
	// rotate car back onto it's wheels after rollover
	void RolloverRecover();

	// set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
	void SetSteering(const T value);

	// get the maximum steering angle in degrees
	T GetMaxSteeringAngle() const;

	const CARTIRE <T> & GetTire(WHEEL_POSITION pos) const {return tire[pos];}
	
	const CARSUSPENSION <T> & GetSuspension(WHEEL_POSITION pos) const {return suspension[pos];}

	MATHVECTOR <T, 3> GetTotalAero() const;
	
	T GetAerodynamicDownforceCoefficient() const;
	
	T GetAeordynamicDragCoefficient() const;

	MATHVECTOR< T, 3 > GetLastBodyForce() const;
	
	T GetFeedback() const;

	void UpdateTelemetry(float dt);

	// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	bool Serialize(joeserialize::Serializer & s);

protected:
// chassis state
	RIGIDBODY <T> body;
	MATHVECTOR <T, 3> center_of_mass;
	COLLISION_WORLD * world;
	btRigidBody * chassis;
	
	// interpolated chassis state
	MATHVECTOR <T, 3> chassisPosition;
	MATHVECTOR <T, 3> chassisCenterOfMass;
	QUATERNION <T> chassisRotation;

// driveline state
	CARFUELTANK <T> fuel_tank;
	CARENGINE <T> engine;
	CARCLUTCH <T> clutch;
	CARTRANSMISSION <T> transmission;
	CARDIFFERENTIAL <T> differential_front;
	CARDIFFERENTIAL <T> differential_rear;
	CARDIFFERENTIAL <T> differential_center;
	std::vector <CARBRAKE <T> > brake;
	std::vector <CARWHEEL <T> > wheel;
	std::vector <CARTIRE <T> > tire;
	
	enum { NONE = 0, FWD = 1, RWD = 2, AWD = 3 } drive;
	T driveshaft_rpm;
	T tacho_rpm;

	bool autoclutch;
	bool autoshift;
	bool shifted;
	int shift_gear;
	T last_auto_clutch;
	T remaining_shift_time;

// traction control state
	bool abs;
	bool tcs;
	std::vector <int> abs_active;
	std::vector <int> tcs_active;
	
// cardynamics state
	std::vector <MATHVECTOR <T, 3> > wheel_velocity;
	std::vector <MATHVECTOR <T, 3> > wheel_position;
	std::vector <QUATERNION <T> > wheel_orientation;
	std::vector <COLLISION_CONTACT> wheel_contact;
	std::vector <CARSUSPENSION <T> > suspension;
	//std::vector <SuspensionConstraint*> new_suspension;

	std::vector <CARAERO <T> > aerodynamics;
	std::list <std::pair <T, MATHVECTOR <T, 3> > > mass_particles;
	
	T maxangle;
	
	T feedback;
	
	MATHVECTOR <T, 3> lastbodyforce; //< held so external classes can extract it for things such as applying physics to camera mounts
	
	std::list <CARTELEMETRY> telemetry;

// chassis, cardynamics
	MATHVECTOR <T, 3> GetDownVector() const;

	// wrappers (to be removed)
	QUATERNION <T> Orientation() const;
	MATHVECTOR <T, 3> Position() const;

	MATHVECTOR <T, 3> LocalToWorld(const MATHVECTOR <T, 3> & local) const;
	
	void ApplyForce(const MATHVECTOR <T, 3> & force);
	
	void ApplyForce(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset);
	
	void ApplyTorque(const MATHVECTOR <T, 3> & torque);

	void UpdateWheelVelocity();
	
	void UpdateWheelTransform();

	// apply engine torque to chassis
	void ApplyEngineTorqueToBody();
	
	// add aerodynamic force / torque to force, torque
	void AddAerodynamics(MATHVECTOR<T, 3> & force, MATHVECTOR<T, 3> & torque);

	// update suspension, return suspension force
	T UpdateSuspension(int i, T dt);

	// apply tire friction to body, return longitudinal tire friction
	T ApplyTireForce(
		int i,
		const T normal_force,
		const QUATERNION <T> & wheel_space);

	// calculate wheel torque
	T CalculateWheelTorque(
		int i,
		const T tire_friction,
		T drive_torque,
		T dt);

	// advance chassis(body, suspension, wheels) simulation by dt
	void UpdateBody(
		const MATHVECTOR <T, 3> & ext_force,
		const MATHVECTOR <T, 3> & ext_torque,
		T drive_torque[],
		T dt);

	// cardynamics
	void Tick(
		MATHVECTOR<T, 3> ext_force,
		MATHVECTOR<T, 3> ext_torque,
		T dt);

	void UpdateWheelContacts();

	void InterpolateWheelContacts();

	void UpdateMass();

// driveline
	// update engine, return wheel drive torque
	void UpdateDriveline(T drive_torque[], T dt);

	// calculate wheel drive torque
	void CalculateDriveTorque(T wheel_drive_torque[], T clutch_torque);

	// calculate driveshaft speed given wheel angular velocity
	T CalculateDriveshaftSpeed();

	// calculate throttle, clutch, gear
	void UpdateTransmission(T dt);

	bool WheelDriven(int i) const;
	
	T AutoClutch(T last_clutch, T dt) const;
	
	T ShiftAutoClutch() const;
	
	T ShiftAutoClutchThrottle(T throttle, T dt);
	
	// calculate next gear based on engine rpm
	int NextGear() const;
	
	// calculate downshift point based on gear, engine rpm
	T DownshiftRPM(int gear) const;

// traction control
	// do traction control system calculations and modify the throttle position if necessary
	void DoTCS(int i, T normal_force);

	// do anti-lock brake system calculations and modify the brake force if necessary
	void DoABS(int i, T normal_force);

// cardynamics initialization
	void Init();

	void GetCollisionBox(
		const btVector3 & chassisSize,
		const btVector3 & chassisCenter,
		btVector3 & center,
		btVector3 & size);

	void InitializeWheelVelocity();

	void AddMassParticle(T mass, MATHVECTOR <T, 3> pos);
};

#endif
