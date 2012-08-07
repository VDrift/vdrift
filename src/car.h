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

	// will align car relative to track surface
	void SetPosition(const MATHVECTOR <float, 3> & position);

    void SetGear(int gear)
	{
	    dynamics.setGear(gear);
	}

	void SetAutoClutch(bool value)
	{
		dynamics.setAutoClutch(value);
	}

	void SetAutoShift(bool value)
	{
		dynamics.setAutoShift(value);
	}

	void ProcessInputs(const std::vector<float> & inputs);

	void Update(double dt);

	const std::vector<CAMERA*> & GetCameras() const
	{
		return cameras;
	}

	int GetWheelCount() const
	{
		return dynamics.getWeelCount();
	}

	MATHVECTOR<float, 3> GetCenterOfMassPosition() const
	{
		return ToMathVector<float>(dynamics.getPosition());
	}

	MATHVECTOR<float, 3> GetPosition() const
	{
		return ToMathVector<float>(motion_state[0].position);
	}

	QUATERNION<float> GetOrientation() const
	{
		return ToMathQuaternion<float>(motion_state[0].rotation);
	}

	MATHVECTOR<float, 3> GetWheelPosition(int i) const
	{
		return ToMathVector<float>(motion_state[i+1].position);
	}

	float GetTireRadius(int i) const
	{
		return dynamics.getWheel(i).getRadius();
	}

	int GetEngineRedline() const
	{
		return dynamics.getEngine().getRedline();
	}

	int GetEngineRPMLimit() const
	{
		return dynamics.getEngine().getRPMLimit();
	}

	bool GetOutOfGas() const
	{
		return dynamics.getOutOfGas();
	}

	float GetNosAmount() const
	{
		return dynamics.getNosAmount();
	}

	bool GetNosActive() const
	{
		return nos_active;
	}

	int GetGear() const
	{
		return dynamics.getTransmission().getGear();
	}

	float GetClutch()
	{
		return dynamics.getClutch().getPosition();
	}

	bool GetABSEnabled() const
	{
		return dynamics.getABSEnabled();
	}

	bool GetABSActive() const
	{
		return dynamics.getABSActive();
	}

	bool GetTCSEnabled() const
	{
		return dynamics.getTCSEnabled();
	}

	bool GetTCSActive() const
	{
		return dynamics.getTCSActive();
	}

	float GetSpeedMPS()
	{
		return dynamics.getSpeedMPS();
	}

	float GetMaxSpeedMPS()
	{
		return dynamics.getMaxSpeedMPS();
	}

	float GetBrakingDistance(float target_velocity)
	{
		return dynamics.getBrakingDistance(target_velocity);
	}

	float GetMaxVelocity(float radius)
	{
		return dynamics.getMaxVelocity(radius);
	}

	const std::string & GetCarType() const
	{
		return cartype;
	}

	void SetSector(int value)
	{
		sector = value;
	}

	int GetSector() const
	{
		return sector;
	}

	const BEZIER * GetCurPatch(int i) const
	{
		return dynamics.getWheel(i).ray.getPatch();
	}

	float GetLastSteer() const
	{
		return last_steer;
	}

	float GetSpeed()
	{
		return dynamics.getSpeed();
	}

	float GetFeedback();

	// returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(int i) const;

	int GetEngineRPM() const
	{
		return dynamics.getTachoRPM();
	}

	int GetEngineStallRPM() const
	{
		return dynamics.getEngine().getStallRPM();
	}

	float GetInvMass() const
	{
		return dynamics.getInvMass();
	}

	MATHVECTOR <float, 3> GetVelocity() const
	{
		return ToMathVector<float>(dynamics.getVelocity());
	}

	// ideal steering angle in degrees
	float GetIdealSteeringAngle() const
	{
		return dynamics.getWheel(0).tire.getIdealSlip();
	}

	// maximum steering angle in degrees
	float GetMaxSteeringAngle() const
	{
		return dynamics.getMaxSteeringAngle();
	}

	// allows to create raycasts
	const btCollisionWorld * GetCollisionWorld() const
	{
		return dynamics.getCollisionWorld();
	}

	sim::Vehicle & GetCarDynamics()
	{
		return dynamics;
	}

	SCENENODE & GetNode()
	{
		return topnode;
	}

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

#endif
