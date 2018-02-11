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
#include "macros.h"

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

	// fixme: move into car input vector?
	void SetSteeringAssist(bool value);
	void SetAutoReverse(bool value);
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

	btVector3 GetTotalAero() const;

	btScalar GetFeedback() const;

	btScalar GetTireSquealAmount(WheelPosition i) const;

	// Maxumum speed for a curve with given radius and friction coefficient
	btScalar GetMaxSpeed(btScalar radius, btScalar friction) const;

	// Distance required to reduce initial to final speed
	btScalar GetBrakeDistance(btScalar initial_speed, btScalar final_speed, btScalar friction) const;

	// This is needed for ray casts in the AI implementation.
	DynamicsWorld * getDynamicsWorld() const {return world;}

	const btCollisionObject & getCollisionObject() const;

	btVector3 LocalToWorld(const btVector3 & local) const;

	// top speed, drivetrain, displacement, power, torque,
	// weight, weight front fraction in metric units
	std::vector<float> GetSpecs() const;

	// print debug info to the given ostream.
	// set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	template <class Stream>
	void DebugPrint(Stream & out, bool p1, bool p2, bool p3, bool p4) const;

	template <class Serializer>
	bool Serialize(Serializer & s);

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
	btAlignedObjectArray<btVector3> wheel_velocity;
	btAlignedObjectArray<btVector3> wheel_position;
	btAlignedObjectArray<btQuaternion> wheel_orientation;

	enum { NONE = 0, FWD = 1, RWD = 2, AWD = 3 } drive;
	btScalar driveshaft_rpm;
	btScalar tacho_rpm;

	// traction control state
	std::vector<int> abs_active;
	std::vector<int> tcs_active;

	// cached coeffs used by CaculateMaxSpeed, GetMaxSpeed and GetBrakeDistance
	btScalar aero_lift_coeff;
	btScalar aero_drag_coeff;
	btScalar lon_friction_coeff;
	btScalar lat_friction_coeff;

	btScalar maxangle;
	btScalar maxspeed;
	btScalar feedback_scale;
	btScalar feedback;

	btScalar brake_value;
	btScalar clutch_value;
	btScalar remaining_shift_time;
	int shift_gear;
	bool shifted;

	// assists
	bool steering_assist;
	bool autoreverse;
	bool autoclutch;
	bool autoshift;
	bool abs;
	bool tcs;

	btVector3 GetDownVector() const;

	btQuaternion LocalToWorld(const btQuaternion & local) const;

	void UpdateWheelVelocity();

	void UpdateWheelTransform();

	void ApplyEngineTorqueToBody(btVector3 & torque);

	void ApplyAerodynamicsToBody(btVector3 & force, btVector3 & torque);

	void ComputeSuspensionDisplacement(int i, btScalar dt);

	// do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
	void DoTCS(int i);

	// do anti-lock brake system calculations and modify the brake force if necessary
	void DoABS(int i);

	void ApplySuspensionForceToBody(int i, btScalar dt, btVector3 & force, btVector3 & torque);

	btVector3 ComputeTireFrictionForce(int i, btScalar normal_force, btScalar rotvel, const btVector3 & linvel, const btQuaternion & wheel_orientation);

	void ApplyWheelForces(int i, btScalar dt, btScalar wheel_drive_torque, btVector3 & force, btVector3 & torque);

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

	btScalar AutoClutch(btScalar clutch_rpm, btScalar dt) const;

	btScalar ShiftAutoClutchThrottle(btScalar throttle, btScalar clutch_rpm, btScalar dt);

	// calculate next gear based on engine rpm
	int NextGear(btScalar clutch_rpm) const;

	// calculate downshift point based on gear, engine rpm
	btScalar DownshiftRPM(int gear) const;

	// max speed in m/s calculated from maxrpm, maxgear, finalgear ratios
	btScalar CalculateMaxSpeed() const;

	// car mass fraction at front wheels
	btScalar CalculateFrontMassRatio() const;

	// total aerodynamic lift and drag coefficients
	void CalculateAerodynamicCoeffs(btScalar & cl, btScalar & cd) const;

	// total longitudinal and lateral tire friction coefficients
	void CalculateFrictionCoeffs(btScalar & mulon, btScalar & mulat) const;

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


template <class Stream>
inline Stream & operator << (Stream & os, const btVector3 & v)
{
	os << v[0] << ", " << v[1] << ", " << v[2];
	return os;
}

template <class Stream>
inline Stream & operator << (Stream & os, const CarTire & tire)
{
	os << "Fx: " << tire.getFx() << "\n";
	os << "Fy: " << tire.getFy() << "\n";
	os << "Slip Ang: " << tire.getSlipAngle() * btScalar(180 / M_PI) << " / ";
	os << tire.getIdealSlipAngle() * btScalar(180 / M_PI) << "\n";
	os << "Slip: " << tire.getSlip() << " / ";
	os << tire.getIdealSlip() << "\n";
	return os;
}

template <class Stream>
inline void CarDynamics::DebugPrint(Stream & out, bool p1, bool p2, bool p3, bool p4) const
{
	if (p1)
	{
		out << "---Body---\n";
		out << "Velocity: " << GetVelocity() << "\n";
		out << "Position: " << GetPosition() << "\n";
		out << "Center of mass: " << -GetCenterOfMassOffset() << "\n";
		out << "Total mass: " << 1 / GetInvMass() << "\n";
		out << "\n";
		fuel_tank.DebugPrint(out);
		out << "\n";
		engine.DebugPrint(out);
		out << "\n";
		clutch.DebugPrint(out);
		out << "\n";
		transmission.DebugPrint(out);
		out << "\n";
		if (drive == RWD)
		{
			out << "(rear)" << "\n";
			differential_rear.DebugPrint(out);
		}
		else if (drive == FWD)
		{
			out << "(front)" << "\n";
			differential_front.DebugPrint(out);
		}
		else if (drive == AWD)
		{
			out << "(center)" << "\n";
			differential_center.DebugPrint(out);

			out << "(front)" << "\n";
			differential_front.DebugPrint(out);

			out << "(rear)" << "\n";
			differential_rear.DebugPrint(out);
		}
		out << "\n";
	}

	if (p2)
	{
		out << "(front left)" << "\n";
		brake[FRONT_LEFT].DebugPrint(out);
		out << "\n";
		suspension[FRONT_LEFT]->DebugPrint(out);
		out << "\n";
		wheel[FRONT_LEFT].DebugPrint(out);
		out << tire[FRONT_LEFT] << "\n";

		out << "(rear left)" << "\n";
		brake[REAR_LEFT].DebugPrint(out);
		out << "\n";
		suspension[REAR_LEFT]->DebugPrint(out);
		out << "\n";
		wheel[REAR_LEFT].DebugPrint(out);
		out << tire[REAR_LEFT] << "\n";
	}

	if (p3)
	{
		out << "(front right)" << "\n";
		brake[FRONT_RIGHT].DebugPrint(out);
		out << "\n";
		suspension[FRONT_RIGHT]->DebugPrint(out);
		out << "\n";
		wheel[FRONT_RIGHT].DebugPrint(out);
		out << tire[FRONT_RIGHT] << "\n";

		out << "(rear right)" << "\n";
		brake[REAR_RIGHT].DebugPrint(out);
		out << "\n";
		suspension[REAR_RIGHT]->DebugPrint(out);
		out << "\n";
		wheel[REAR_RIGHT].DebugPrint(out);
		out << tire[REAR_RIGHT] << "\n";
	}

	if (p4)
	{
		for (int i = 0; i != aerodevice.size(); ++i)
		{
			out << "---Aerodynamic Device---" << "\n";
			out << "Drag: " << aerodevice[i].getDrag() << "\n";
			out << "Lift: " << aerodevice[i].getLift() << "\n\n";
		}
	}
}


#define _SERIALIZEX_(s,var) if (!Serializex(s,var)) return false

template <class Serializer>
inline bool Serializex(Serializer & s, btQuaternion & q)
{
	_SERIALIZE_(s, q[0]);
	_SERIALIZE_(s, q[1]);
	_SERIALIZE_(s, q[2]);
	_SERIALIZE_(s, q[3]);
	return true;
}

template <class Serializer>
inline bool Serializex(Serializer & s, btVector3 & v)
{
	_SERIALIZE_(s, v[0]);
	_SERIALIZE_(s, v[1]);
	_SERIALIZE_(s, v[2]);
	return true;
}

template <class Serializer>
inline bool Serializex(Serializer & s, btMatrix3x3 & m)
{
	_SERIALIZEX_(s, m[0]);
	_SERIALIZEX_(s, m[1]);
	_SERIALIZEX_(s, m[2]);
	return true;
}

template <class Serializer>
inline bool Serializex(Serializer & s, btTransform & t)
{
	_SERIALIZEX_(s, t.getBasis());
	_SERIALIZEX_(s, t.getOrigin());
	return true;
}

template <class Serializer>
inline bool Serializex(Serializer & s, btRigidBody & b)
{
	btTransform t = b.getCenterOfMassTransform();
	btVector3 v = b.getLinearVelocity();
	btVector3 w = b.getAngularVelocity();
	_SERIALIZEX_(s, t);
	_SERIALIZEX_(s, v);
	_SERIALIZEX_(s, w);
	b.setCenterOfMassTransform(t);
	b.setLinearVelocity(v);
	b.setAngularVelocity(w);
	return true;
}

template <class Serializer>
inline bool CarDynamics::Serialize(Serializer & s)
{
	_SERIALIZE_(s, engine);
	_SERIALIZE_(s, clutch);
	_SERIALIZE_(s, transmission);
	_SERIALIZE_(s, differential_front);
	_SERIALIZE_(s, differential_rear);
	_SERIALIZE_(s, differential_center);
	_SERIALIZE_(s, fuel_tank);
	_SERIALIZE_(s, abs);
	_SERIALIZE_(s, abs_active);
	_SERIALIZE_(s, tcs);
	_SERIALIZE_(s, tcs_active);
	_SERIALIZE_(s, clutch_value);
	_SERIALIZE_(s, remaining_shift_time);
	_SERIALIZE_(s, shift_gear);
	_SERIALIZE_(s, shifted);
	_SERIALIZE_(s, autoshift);
	_SERIALIZEX_(s, *(btRigidBody*)body);
	_SERIALIZEX_(s, transform);
	_SERIALIZEX_(s, linear_velocity);
	_SERIALIZEX_(s, angular_velocity);
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		_SERIALIZE_(s, wheel[i]);
		_SERIALIZE_(s, brake[i]);
		//_SERIALIZE_(s, tire[i]);
		_SERIALIZE_(s, *suspension[i]);
		_SERIALIZEX_(s, wheel_velocity[i]);
		_SERIALIZEX_(s, wheel_position[i]);
		_SERIALIZEX_(s, wheel_orientation[i]);
	}
	return true;
}

#endif
