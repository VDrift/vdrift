#ifndef _CARDYNAMICS_H
#define _CARDYNAMICS_H

#include "mathvector.h"
#include "quaternion.h"
#include "rigidbody.h"
#include "collision_contact.h"
#include "carpowertrain.h"
#include "carsuspension.h"
#include "carwheelposition.h"
#include "caraerodynamicdevice.h"
#include "cartelemetry.h"
#include "joeserialize.h"
#include "macros.h"
#include "BulletDynamics/Dynamics/btActionInterface.h"

class CONFIGFILE;
class COLLISION_WORLD;

class CARDYNAMICS : public btActionInterface
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	typedef double T;
	
	CARDYNAMICS();
	
	bool Load(
		CONFIGFILE & c,
		const std::string & sharedpartspath,
		std::ostream & error_output);

	void Init(
		COLLISION_WORLD & world,
		const MATHVECTOR <T, 3> chassisSize,
		const MATHVECTOR <T, 3> chassisCenter,
		const MATHVECTOR <T, 3> & position,
		const QUATERNION <T> & orientation);

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
	MATHVECTOR <T, 3> GetEnginePosition() const;

// collision world interface
	const COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp) const;
	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wp);

// chassis
	float GetMass() const;
	T GetSpeed() const;
	MATHVECTOR <T, 3> GetVelocity() const;

// powertrain
	// input
	void StartEngine() {powertrain.StartEngine();}
	void ShiftGear(int value) {powertrain.ShiftGear(value);}
	void SetThrottle(float value) {powertrain.SetThrottle(value);}
	void SetClutch(float value) {powertrain.SetClutch(value);}
	void SetBrake(float value) {powertrain.SetBrake(value);}
	void SetHandBrake(float value) {powertrain.SetHandBrake(value);}
	void SetAutoClutch(bool value) {powertrain.SetAutoClutch(value);}
	void SetAutoShift(bool value) {powertrain.SetAutoShift(value);}
	void SetABS(bool value) {powertrain.SetABS(value);}
	void SetTCS(bool value) {powertrain.SetTCS(value);}

	// output
	T GetSpeedMPS() const {return powertrain.GetSpeedMPS();}
	T GetTachoRPM() const {return powertrain.GetTachoRPM();}
	bool GetABSEnabled() const {return powertrain.GetABSEnabled();}
	bool GetABSActive() const {return powertrain.GetABSActive();}
	bool GetTCSEnabled() const {return powertrain.GetABSEnabled();}
	bool GetTCSActive() const {return powertrain.GetTCSActive();}

	// state access
	const CARENGINE <T> & GetEngine() const {return powertrain.GetEngine();}
	const CARCLUTCH <T> & GetClutch() const {return powertrain.GetClutch();}
	const CARTRANSMISSION <T> & GetTransmission() const {return powertrain.GetTransmission();}
	const CARBRAKE <T> & GetBrake(WHEEL_POSITION pos) const {return powertrain.GetBrake(pos);}
	const CARWHEEL <T> & GetWheel(WHEEL_POSITION pos) const {return powertrain.GetWheel(pos);}
	const CARTIRE <T> & GetTire(WHEEL_POSITION pos) const {return powertrain.GetTire(pos);}

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
	
	CARPOWERTRAIN <T> powertrain;

// suspension
	std::vector <MATHVECTOR <T, 3> > wheel_velocity;
	std::vector <MATHVECTOR <T, 3> > wheel_position;
	std::vector <QUATERNION <T> > wheel_orientation;
	std::vector <COLLISION_CONTACT> wheel_contact;
	std::vector <CARSUSPENSION <T> > suspension;

// others
	T maxangle;
	T feedback;
	CARTELEMETRY telemetry;
	MATHVECTOR <T, 3> lastbodyforce; //< held so external classes can extract it for things such as applying physics to camera mounts
	std::vector <CARAERO <T> > aerodynamics;
	std::list <std::pair <T, MATHVECTOR <T, 3> > > mass_particles;
	
// chassis, cardynamics
	MATHVECTOR <T, 3> GetDownVector() const;
	
	QUATERNION <T> LocalToWorld(const QUATERNION <T> & local) const;
	
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
	T ApplyTireForce(WHEEL_POSITION pos, const T normal_force, const QUATERNION <T> & wheel_space);

	// advance chassis(body, suspension, wheels) simulation by dt
	void UpdateBody(const MATHVECTOR <T, 3> & ext_force, const MATHVECTOR <T, 3> & ext_torque, T dt);

	// cardynamics
	void Tick(MATHVECTOR<T, 3> ext_force, MATHVECTOR<T, 3> ext_torque, T dt);

	void UpdateWheelContacts();

	void InterpolateWheelContacts();

	void UpdateMass();

// cardynamics initialization
	void Init();

	void GetCollisionBox(
		const btVector3 & chassisSize,
		const btVector3 & chassisCenter,
		btVector3 & center,
		btVector3 & size);

	void AddMassParticle(T mass, MATHVECTOR <T, 3> pos);
};

#endif
