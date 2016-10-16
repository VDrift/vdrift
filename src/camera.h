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

#ifndef _CAMERA_H
#define _CAMERA_H

#include "minmax.h"
#include "mathvector.h"
#include "quaternion.h"
#include "coordinatesystem.h"
#include <string>

///base class for a camera
class Camera
{
public:
	Camera(const std::string & camera_name) : name(camera_name), fov(0) {}

	virtual ~Camera() {}

	const std::string & GetName() const { return name; }

	void SetFOV(float value) { fov = Clamp(value, 0.0f, 120.0f); }

	float GetFOV() const { return fov; }

	virtual const Vec3 & GetPosition() const { return position; }

	virtual const Quat & GetOrientation() const { return rotation; }

	// reset position, orientation
	virtual void Reset(const Vec3 & /*newpos*/, const Quat & /*newquat*/) {};

	// update position, orientation
	virtual void Update(const Vec3 & /*newpos*/, const Quat & /*newquat*/, float /*dt*/) {};

	// move relative to current position, orientation
	virtual void Move(float /*dx*/, float /*dy*/, float /*dz*/) {};

	// rotate relative to current position, orientation
	virtual void Rotate(float /*up*/, float /*left*/) {};

protected:
	const std::string name;
	Vec3 position;
	Quat rotation;
	float fov;
};

inline float AngleBetween(Vec3 vec1, Vec3 vec2)
{
	float dotprod = vec1.Normalize().dot(vec2.Normalize());
	float angle = std::acos(dotprod);
	float epsilon = 1E-6f;
	if (std::abs(dotprod) <= epsilon) angle = M_PI_2;
	if (dotprod >= 1-epsilon) angle = 0;
	if (dotprod <= epsilon-1) angle = M_PI;
	return angle;
}

inline Quat LookAt(
	Vec3 eye,
	Vec3 center,
	Vec3 up)
{
	Quat rotation;

	Vec3 forward(center - eye);
	forward = forward.Normalize();
	Vec3 side = (forward.cross(up)).Normalize();
	Vec3 realup = side.cross(forward);

	//rotate so the camera is pointing along the forward line
	float theta = AngleBetween(forward, Direction::Forward);
	assert(theta == theta);
	if (std::abs(theta) > 1E-3f)
	{
		Vec3 axis = forward.cross(Direction::Forward).Normalize();
		rotation.Rotate(-theta, axis[0], axis[1], axis[2]);
	}

	//now rotate the camera so it's pointing up
	Vec3 curup = Direction::Up;
	rotation.RotateVector(curup);

	float rollangle = AngleBetween(realup, curup);
	if (curup.dot(side) > 0)
	{
		rollangle = float(2 * M_PI) - rollangle;
	}
	assert(rollangle == rollangle);

	Vec3 axis = forward;
	rotation.Rotate(rollangle, axis[0], axis[1], axis[2]);

	return rotation;
}

#endif // _CAMERA_H
