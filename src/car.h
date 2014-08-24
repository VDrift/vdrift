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

#include "physics/cardynamics.h"
#include "cargraphics.h"
#include "carsound.h"
#include "tobullet.h"
#include "joeserialize.h"
#include "macros.h"

#include <iosfwd>
#include <string>
#include <vector>

class Bezier;
class PTree;
class ContentManager;

class Car
{
public:
	Car();

	virtual ~Car();

	bool LoadGraphics(
		const PTree & cfg,
		const std::string & carpath,
		const std::string & carname,
		const std::string & carwheel,
		const std::string & carpaint,
		const Vec3 & carcolor,
		const int anisotropy,
		const float camerabounce,
		ContentManager & content,
		std::ostream & error_output);

	bool LoadSounds(
		const std::string & carpath,
		const std::string & carname,
		Sound & soundsystem,
		ContentManager & content,
		std::ostream & error_output);

	bool LoadPhysics(
		std::ostream & error_output,
		ContentManager & content,
		DynamicsWorld & world,
		const PTree & cfg,
		const std::string & carpath,
		const std::string & cartire,
		const Vec3 & position,
		const Quat & orientation,
		const bool defaultabs,
		const bool defaulttcs,
		const bool damage);

	/// update car sdtate from car inputs
	void Update(const std::vector <float> & inputs);

	/// update car state from car dynamics
	void Update(double dt);

	void SetInteriorView(bool value);

	void SetColor(float r, float g, float b)
	{
		graphics.SetColor(r, g, b);
	}

	const Vec3 GetWheelPosition(const WheelPosition wpos) const
	{
		return ToMathVector<float>(dynamics.GetWheelPosition(wpos));
	}

	float GetWheelRadius(const WheelPosition wpos) const
	{
		return dynamics.GetWheel(wpos).GetRadius();
	}

	CollisionContact & GetWheelContact(WheelPosition wheel_index)
	{
		return dynamics.GetWheelContact(wheel_index);
	}

	const std::vector<Camera*> & GetCameras() const
	{
		return graphics.GetCameras();
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
		return nos_active;
	}

	int GetGear() const
	{
		return dynamics.GetTransmission().GetGear();
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

	const Bezier * GetCurPatch(WheelPosition wheel) const
	{
		return dynamics.GetWheelContact(wheel).GetPatch();
	}

	float GetSpeed()
	{
		return dynamics.GetSpeed();
	}

	float GetFeedback() const
	{
		return dynamics.GetFeedback() / mz_nominalmax;
	}

	// returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(WheelPosition i) const
	{
		return dynamics.GetTireSquealAmount(i);
	}

	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const
	{
		dynamics.DebugPrint(out, p1, p2, p3, p4);
	}

	int GetEngineRPM() const
	{
		return dynamics.GetTachoRPM();
	}

	int GetEngineStallRPM() const
	{
		return dynamics.GetEngine().GetStallRPM();
	}

	// interpoated position
	Vec3 GetCenterOfMassPosition() const
	{
		return ToMathVector<float>(dynamics.GetCenterOfMass());
	}

	// interpolated position
	Vec3 GetPosition() const
	{
		return ToMathVector<float>(dynamics.GetPosition());
	}

	// interpolated orientation
	Quat GetOrientation() const
	{
		return ToQuaternion<float>(dynamics.GetOrientation());
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

	Vec3 GetVelocity() const
	{
		return ToMathVector<float>(dynamics.GetVelocity());
	}

	float GetTireMaxFx(WheelPosition tire_index) const
	{
		return dynamics.GetTire(tire_index).getMaxFx(0.25*9.81/GetInvMass());
	}

	float GetTireMaxFy(WheelPosition tire_index) const
	{
		return dynamics.GetTire(tire_index).getMaxFy(0.25*9.81/GetInvMass(), 0.0);
	}

	// optimum steering angle in degrees
	float GetOptimumSteeringAngle() const
	{
		return dynamics.GetTire(FRONT_LEFT).getIdealSlipAngle() * SIMD_DEGS_PER_RAD;
	}

	// maximum steering angle in degrees
	float GetMaxSteeringAngle() const
	{
		return dynamics.GetMaxSteeringAngle();
	}

	// allows to create raycasts
	DynamicsWorld * GetDynamicsWorld()
	{
		return dynamics.getDynamicsWorld();
	}

	CarDynamics & GetCarDynamics()
	{
		return dynamics;
	}

	const CarDynamics & GetCarDynamics() const
	{
		return dynamics;
	}

	SceneNode & GetNode()
	{
		return graphics.GetNode();
	}

	bool Serialize(joeserialize::Serializer & s);

protected:
	friend class joeserialize::Serializer;

	std::string cartype;
	CarDynamics dynamics;
	CarGraphics graphics;
	CarSound sound;

	// internal variables that might change during driving (so, they need to be serialized)
	bool nos_active;

	// nominal maximum Mz force, used to scale force feedback
	float mz_nominalmax;

	// last lap timing sector that the car hit
	int sector;

	// last bezier patch that each wheel hit
	const Bezier * curpatch[WHEEL_POSITION_SIZE];
};

#endif
