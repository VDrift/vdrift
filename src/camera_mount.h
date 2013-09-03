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

#ifndef _CAMERA_MOUNT_H
#define _CAMERA_MOUNT_H

#include "camera.h"
#include "random.h"

class CameraMount : public Camera
{
public:
	CameraMount(const std::string & name);

	void Reset(const Vec3 & newpos, const Quat & newquat);

	void Update(const Vec3 & newpos, const Quat & newdir, float dt);

	void SetOffset(const Vec3 & lookfrom, const Vec3 & lookat)
	{
		offset = lookfrom;
		offsetrot = LookAt(lookfrom, lookat, Direction::Up);
	}

	void SetStiffness(float value)
	{
		stiffness = value;
	}

	void SetEffectStrength(float value)
	{
		offset_effect_strength = value;
	}

private:
	Quat offsetrot;
	Vec3 offset;
	Vec3 displacement;
	Vec3 velocity;
	float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
	float mass;
	float offset_effect_strength; ///< where 1.0 is normal effect strength
	float effect;

	DeterministicRandom randgen;

	void UpdatePosition(const Vec3 & newpos);
};

#endif // _CAMERA_MOUNT_H
