#ifndef _CAR_H
#define _CAR_H

#include "cardynamics.h"
#include "tobullet.h"
#include "scenenode.h"
#include "soundsource.h"
#include "camera_system.h"
#include "joeserialize.h"
#include "macros.h"
#include "suspensionbumpdetection.h"
#include "crashdetection.h"
#include "enginesoundinfo.h"

class BEZIER;
class PERFORMANCE_TESTING;
class MODEL;
class ContentManager;
class PTree;

class CAR
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	CAR();

	bool LoadGraphics(
		const PTree & cfg,
		const std::string & carpath,
		const std::string & carname,
		const std::string & partspath,
		const MATHVECTOR <float, 3> & carcolor,
		const std::string & carpaint,
		const int anisotropy,
		const float camerabounce,
		const bool damage,
		const bool debugmode,
		ContentManager & content,
		std::ostream & info_output,
		std::ostream & error_output);

	bool LoadSounds(
		const std::string & carpath,
		const std::string & carname,
		ContentManager & content,
		std::ostream & info_output,
		std::ostream & error_output);

	bool LoadPhysics(
		const PTree & cfg,
		const std::string & carpath,
		const MATHVECTOR <float, 3> & position,
		const QUATERNION <float> & orientation,
		const bool defaultabs,
		const bool defaulttcs,
		const bool damage,
		ContentManager & content,
		DynamicsWorld & world,
		std::ostream & info_output,
		std::ostream & error_output);

	// change car color
	void SetColor(float r, float g, float b);

	// will align car relative to track surface
	void SetPosition(const MATHVECTOR <float, 3> & position);

	void Update(double dt);

	void GetSoundList(std::list <SOUNDSOURCE *> & outputlist);

	void GetEngineSoundList(std::list <SOUNDSOURCE *> & outputlist);

	// interpolated
	const MATHVECTOR <float, 3> GetWheelPosition(const WHEEL_POSITION wpos) const
	{
		return ToMathVector<float>(dynamics.GetWheelPosition(wpos));
	}

	float GetTireRadius(const WHEEL_POSITION wpos) const
	{
		return dynamics.GetTire(wpos).GetRadius();
	}

	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wheel_index)
	{
		return dynamics.GetWheelContact(wheel_index);
	}

	void HandleInputs(const std::vector <float> & inputs, float dt);

	CAMERA_SYSTEM & Cameras()
	{
		return cameras;
	}

	int GetEngineRedline() const
	{
		return dynamics.GetEngine().GetRedline();
	}

	int GetEngineRPMLimit() const
	{
		return dynamics.GetEngine().GetRPMLimit();
	}

	bool GetOutOfGas() const
	{
		return dynamics.GetOutOfGas();
	}

	float GetNosAmount() const
	{
		return dynamics.GetNosAmount();
	}

	bool GetNosActive() const
	{
		return nosactive;
	}

	int GetGear() const
	{
		return dynamics.GetTransmission().GetGear();
	}

    void SetGear(int gear)
	{
	    dynamics.ShiftGear(gear);
	}

	float GetClutch()
	{
		return dynamics.GetClutch().GetClutch();
	}

	void SetAutoClutch(bool value)
	{
		dynamics.SetAutoClutch(value);
	}

	void SetAutoShift(bool value)
	{
		dynamics.SetAutoShift(value);
	}

	bool GetABSEnabled() const
	{
		return dynamics.GetABSEnabled();
	}

	bool GetABSActive() const
	{
		return dynamics.GetABSActive();
	}

	bool GetTCSEnabled() const
	{
		return dynamics.GetTCSEnabled();
	}

	bool GetTCSActive() const
	{
		return dynamics.GetTCSActive();
	}

	float GetSpeedMPS()
	{
		return dynamics.GetSpeedMPS();
	}

	float GetMaxSpeedMPS()
	{
		return dynamics.GetMaxSpeedMPS();
	}

	std::string GetCarType() const
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

	const BEZIER * GetCurPatch(WHEEL_POSITION wheel) const
	{
		return dynamics.GetWheelContact(wheel).GetPatch();
	}

	float GetLastSteer() const
	{
		return last_steer;
	}

	float GetSpeed()
	{
		return dynamics.GetSpeed();
	}

	MATHVECTOR <float, 3> GetTotalAero() const
	{
		return ToMathVector<float>(dynamics.GetTotalAero());
	}

	float GetFeedback();

	// returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(WHEEL_POSITION i) const;

	void EnableGlass(bool enable);

	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const
	{
		dynamics.DebugPrint(out, p1, p2, p3, p4);
	}

	bool Serialize(joeserialize::Serializer & s);

/// AI interface
	int GetEngineRPM() const
	{
		return dynamics.GetTachoRPM();
	}

	int GetEngineStallRPM() const
	{
		return dynamics.GetEngine().GetStallRPM();
	}

	// interpoated position
	MATHVECTOR <float, 3> GetCenterOfMassPosition() const
	{
		return ToMathVector<float>(dynamics.GetCenterOfMass());
	}

	// interpolated position
	MATHVECTOR <float, 3> GetPosition() const
	{
		return ToMathVector<float>(dynamics.GetPosition());
	}

	// interpolated orientation
	QUATERNION <float> GetOrientation() const
	{
		return ToMathQuaternion<float>(dynamics.GetOrientation());
	}

	float GetAerodynamicDownforceCoefficient() const
	{
		return dynamics.GetAerodynamicDownforceCoefficient();
	}

	float GetAeordynamicDragCoefficient() const
	{
		return dynamics.GetAeordynamicDragCoefficient();
	}

	float GetInvMass() const
	{
		return dynamics.GetInvMass();
	}

	MATHVECTOR <float, 3> GetVelocity() const
	{
		return ToMathVector<float>(dynamics.GetVelocity());
	}

	float GetTireMaxFx(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaxFx(0.25*9.81/GetInvMass());
	}

	float GetTireMaxFy(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaxFy(0.25*9.81/GetInvMass(), 0.0);
	}

	float GetTireMaxMz(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaxMz(0.25*9.81/GetInvMass(), 0.0);
	}

	// optimum steering angle in degrees
	float GetOptimumSteeringAngle() const
	{
		return dynamics.GetTire(FRONT_LEFT).GetIdealSlip();
	}

	// maximum steering angle in degrees
	float GetMaxSteeringAngle() const
	{
		return dynamics.GetMaxSteeringAngle();
	}

	SCENENODE & GetNode() {return topnode;}

protected:
	SCENENODE topnode;
	CARDYNAMICS dynamics;

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

	SUSPENSIONBUMPDETECTION suspensionbumpdetection[4];
	CRASHDETECTION crashdetection;
	CAMERA_SYSTEM cameras;

	std::list<std::pair <ENGINESOUNDINFO, SOUNDSOURCE> > enginesounds;
	SOUNDSOURCE tiresqueal[WHEEL_POSITION_SIZE];
	SOUNDSOURCE tirebump[WHEEL_POSITION_SIZE];
	SOUNDSOURCE grasssound[WHEEL_POSITION_SIZE];
	SOUNDSOURCE gravelsound[WHEEL_POSITION_SIZE];
	SOUNDSOURCE crashsound;
	SOUNDSOURCE gearsound;
	SOUNDSOURCE brakesound;
	SOUNDSOURCE handbrakesound;
	SOUNDSOURCE roadnoise;

	int gearsound_check;
	bool brakesound_check;
	bool handbrakesound_check;

	// steering wheel
	QUATERNION<float> steer_orientation;
	QUATERNION<float> steer_rotation;
	float steer_angle_max;

	//internal variables that might change during driving (so, they need to be serialized)
	float last_steer;
	bool lookbehind;
	bool nosactive;

	std::string cartype;
	int sector; //the last lap timing sector that the car hit
	const BEZIER * curpatch[WHEEL_POSITION_SIZE]; //the last bezier patch that each wheel hit

	float applied_brakes; ///< cached so we can update the brake light

	float mz_nominalmax; //the nominal maximum Mz force, used to scale force feedback

	void UpdateSounds(float dt);

	void UpdateCameras(float dt);

	void UpdateGraphics();

	bool LoadLight(
		const PTree & cfg,
		ContentManager & content,
		std::ostream & error_output);
};

#endif
