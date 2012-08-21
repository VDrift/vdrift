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

#ifndef _CAR_H
#define _CAR_H

#include "physics/vehicle.h"
#include "physics/motionstate.h"
#include "tobullet.h"
#include "graphics/scenenode.h"
#include "crashdetection.h"
#include "enginesoundinfo.h"
#include "joeserialize.h"
#include "macros.h"

class BEZIER;
class CAMERA;
class PERFORMANCE_TESTING;
class MODEL;
class SOUND;
class ContentManager;
class PTree;
class btCollisionWorld;

class CAR
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	CAR();

	virtual ~CAR();

	bool LoadGraphics(
		const PTree & cfg,
		const std::string & carpath,
		const std::string & carname,
		const std::string & carpaint,
		const MATHVECTOR <float, 3> & carcolor,
		const int anisotropy,
		const float camerabounce,
		ContentManager & content,
		std::ostream & error);

	bool LoadSounds(
		const PTree & cfg,
		const std::string & carpath,
		const std::string & carname,
		SOUND & sound,
		ContentManager & content,
		std::ostream & error);

	bool LoadPhysics(
		const PTree & cfg,
		const std::string & carpath,
		const MATHVECTOR <float, 3> & position,
		const QUATERNION <float> & orientation,
		const bool defaultabs,
		const bool defaulttcs,
		const bool damage,
		sim::World & world,
		ContentManager & content,
		std::ostream & error);

	// change car color
	void SetColor(float r, float g, float b);

	void SetInteriorView(bool value);

    void SetGear(int gear);

	void SetAutoClutch(bool value);

	void SetAutoShift(bool value);

	void ProcessInputs(const std::vector<float> & inputs);

	void Update(double dt);

	const std::vector<CAMERA*> & GetCameras() const;

	/// interpolated car center position
	MATHVECTOR<float, 3> GetPosition() const;

	/// interpolated car center orientation
	QUATERNION<float> GetOrientation() const;

	/// interpolated wheel position
	MATHVECTOR<float, 3> GetWheelPosition(int i) const;

	float GetTireRadius(int i) const;

	int GetWheelCount() const;

	int GetEngineRedline() const;

	int GetEngineRPMLimit() const;

	float GetFuelAmount() const;

	float GetNosAmount() const;

	bool GetNosActive() const;

	int GetGear() const;

	float GetClutch();

	bool GetABSEnabled() const;

	bool GetABSActive() const;

	bool GetTCSEnabled() const;

	bool GetTCSActive() const;

	float GetSpeedMPS();

	float GetMaxSpeedMPS();

	const std::string & GetCarType() const;

	void SetSector(int value);

	int GetSector() const;

	const BEZIER * GetCurPatch(int i) const;

	float GetLastSteer() const;

	float GetSpeed();

	float GetFeedback();

	// returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(int i) const;

	int GetEngineRPM() const;

	int GetEngineStallRPM() const;

	float GetInvMass() const;

	MATHVECTOR <float, 3> GetVelocity() const;

	float GetBrakingDistance(float target_velocity);

	float GetMaxVelocity(float radius);

	// ideal steering angle in degrees
	float GetIdealSteeringAngle() const;

	// maximum steering angle in degrees
	float GetMaxSteeringAngle() const;

	// allows to create raycasts
	const btCollisionWorld * GetCollisionWorld() const;

	sim::Vehicle & GetCarDynamics();

	MATHVECTOR<float, 3> GetCenterOfMassPosition() const;

	SCENENODE & GetNode();

	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const;

	bool Serialize(joeserialize::Serializer & s);

protected:
	SCENENODE topnode;

	// body + n wheels + m children shapes
	btAlignedObjectArray<sim::MotionState> motion_state;
	sim::Vehicle dynamics;

	keyed_container<SCENENODE>::handle bodynode;
	keyed_container<SCENENODE>::handle steernode;
	keyed_container<DRAWABLE>::handle brakelights;
	keyed_container<DRAWABLE>::handle reverselights;

	struct LIGHT
	{
		keyed_container<SCENENODE>::handle node;
		keyed_container<DRAWABLE>::handle draw;
	};
	std::list<LIGHT> lights;
	std::list<std::tr1::shared_ptr<MODEL> > models;

	CRASHDETECTION crashdetection;
	std::vector<CAMERA*> cameras;

	std::vector<ENGINESOUNDINFO> enginesounds;
	std::vector<size_t> roadsound;
	std::vector<size_t> gravelsound;
	std::vector<size_t> grasssound;
	std::vector<size_t> bumpsound;
	size_t crashsound;
	size_t gearsound;
	size_t brakesound;
	size_t handbrakesound;
	size_t roadnoise;
	SOUND * psound;

	int gearsound_check;
	bool brakesound_check;
	bool handbrakesound_check;

	// steering wheel
	QUATERNION<float> steer_orientation;
	QUATERNION<float> steer_rotation;
	float steer_angle_max;

	// internal variables that might change during driving (so, they need to be serialized)
	float last_steer;
	bool nos_active;
	bool driver_view;

	std::string cartype;
	int sector; //the last lap timing sector that the car hit
	std::vector<const BEZIER *> curpatch; //the last bezier patch that each wheel hit

	float applied_brakes; // cached so we can update the brake light

	float mz_nominalmax; //the nominal maximum Mz force, used to scale force feedback

	void RemoveSounds();

	void UpdateSounds(float dt);

	void UpdateGraphics();

	bool LoadLight(
		const PTree & cfg,
		ContentManager & content,
		std::ostream & error);
};

// implementation

inline void CAR::SetGear(int gear)
{
	dynamics.setGear(gear);
}

inline void CAR::SetAutoClutch(bool value)
{
	dynamics.setAutoClutch(value);
}

inline void CAR::SetAutoShift(bool value)
{
	dynamics.setAutoShift(value);
}

inline const std::vector<CAMERA*> & CAR::GetCameras() const
{
	return cameras;
}

inline MATHVECTOR<float, 3> CAR::GetPosition() const
{
	return cast(motion_state[0].position);
}

inline QUATERNION<float> CAR::GetOrientation() const
{
	return cast(motion_state[0].rotation);
}

inline MATHVECTOR<float, 3> CAR::GetWheelPosition(int i) const
{
	return cast(motion_state[i+1].position);
}

inline float CAR::GetTireRadius(int i) const
{
	return dynamics.getWheel(i).getRadius();
}

inline int CAR::GetWheelCount() const
{
	return dynamics.getWeelCount();
}

inline int CAR::GetEngineRedline() const
{
	return dynamics.getEngine().getRedline();
}

inline int CAR::GetEngineRPMLimit() const
{
	return dynamics.getEngine().getRPMLimit();
}

inline float CAR::GetFuelAmount() const
{
	return dynamics.getFuelAmount();
}

inline float CAR::GetNosAmount() const
{
	return dynamics.getNosAmount();
}

inline bool CAR::GetNosActive() const
{
	return nos_active;
}

inline int CAR::GetGear() const
{
	return dynamics.getTransmission().getGear();
}

inline float CAR::GetClutch()
{
	return dynamics.getClutch().getPosition();
}

inline bool CAR::GetABSEnabled() const
{
	return dynamics.getABSEnabled();
}

inline bool CAR::GetABSActive() const
{
	return dynamics.getABSActive();
}

inline bool CAR::GetTCSEnabled() const
{
	return dynamics.getTCSEnabled();
}

inline bool CAR::GetTCSActive() const
{
	return dynamics.getTCSActive();
}

inline float CAR::GetSpeedMPS()
{
	return dynamics.getSpeedMPS();
}

inline float CAR::GetMaxSpeedMPS()
{
	return dynamics.getMaxSpeedMPS();
}

inline const std::string & CAR::GetCarType() const
{
	return cartype;
}

inline void CAR::SetSector(int value)
{
	sector = value;
}

inline int CAR::GetSector() const
{
	return sector;
}

inline const BEZIER * CAR::GetCurPatch(int i) const
{
	return dynamics.getWheel(i).ray.getPatch();
}

inline float CAR::GetLastSteer() const
{
	return last_steer;
}

inline float CAR::GetSpeed()
{
	return dynamics.getSpeed();
}

inline int CAR::GetEngineRPM() const
{
	return dynamics.getTachoRPM();
}

inline int CAR::GetEngineStallRPM() const
{
	return dynamics.getEngine().getStallRPM();
}

inline float CAR::GetInvMass() const
{
	return dynamics.getInvMass();
}

inline MATHVECTOR <float, 3> CAR::GetVelocity() const
{
	return cast(dynamics.getVelocity());
}

inline float CAR::GetBrakingDistance(float target_velocity)
{
	return dynamics.getBrakingDistance(target_velocity);
}

inline float CAR::GetMaxVelocity(float radius)
{
	return dynamics.getMaxVelocity(radius);
}

inline float CAR::GetIdealSteeringAngle() const
{
	return dynamics.getWheel(0).tire.getIdealSlip();
}

inline float CAR::GetMaxSteeringAngle() const
{
	return dynamics.getMaxSteeringAngle();
}

inline const btCollisionWorld * CAR::GetCollisionWorld() const
{
	return dynamics.getCollisionWorld();
}

inline sim::Vehicle & CAR::GetCarDynamics()
{
	return dynamics;
}

inline MATHVECTOR<float, 3> CAR::GetCenterOfMassPosition() const
{
	return cast(dynamics.getPosition());
}

inline SCENENODE & CAR::GetNode()
{
	return topnode;
}

#endif
