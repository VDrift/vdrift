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

#include "camera_orbit.h"
#include "coordinatesystem.h"

CameraOrbit::CameraOrbit(const std::string & name) :
	Camera(name),
	offset(-Direction::Forward * 3),
	distance(-1.5f * Direction::Forward.dot(offset)),
	leftright_rotation(0),
	updown_rotation(0)
{
	rotation.LoadIdentity();
}

void CameraOrbit::SetOffset(const Vec3 & value)
{
	offset = value;
	if (offset.dot(Direction::Forward) > -1E-3f)
	{
		offset = offset - Direction::Forward;
	}
}

void CameraOrbit::Reset(const Vec3 & newfocus, const Quat &)
{
	focus = newfocus;
	distance = -1.5f * Direction::Forward.dot(offset);
	leftright_rotation = 0;
	updown_rotation = 0;
	rotation.LoadIdentity();
	position = offset;
}

void CameraOrbit::Update(const Vec3 & newfocus, const Quat &, float)
{
	focus = newfocus;
}

void CameraOrbit::Rotate(float up, float left)
{
	updown_rotation = Clamp(updown_rotation + up, -1.5f, 1.5f);

	leftright_rotation -= left;

	rotation.LoadIdentity();
	rotation.Rotate(updown_rotation, Direction::Right);
	rotation.Rotate(leftright_rotation, Direction::Up);
}

void CameraOrbit::Move(float dx, float dy, float dz)
{
	Vec3 move(dx, dy, dz);
	distance += Direction::Forward.dot(move);
	float min_distance = -Direction::Forward.dot(offset);
	float max_distance = 4 * min_distance;
	distance = Clamp(distance, min_distance, max_distance);

	position = -Direction::Forward * distance;
	rotation.RotateVector(position);
	position = position + focus;
}
