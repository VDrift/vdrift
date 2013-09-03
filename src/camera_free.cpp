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

#include "camera_free.h"
#include "coordinatesystem.h"

CameraFree::CameraFree(const std::string & name) :
	Camera(name),
	offset(Direction::Up * 2 - Direction::Forward * 8),
	leftright_rotation(0),
	updown_rotation(0)
{
	rotation.LoadIdentity();
}

void CameraFree::SetOffset(const Vec3 & value)
{
	offset = value;
	if (offset.dot(Direction::Forward) < 0.001)
	{
		offset = offset - Direction::Forward;
	}
}

void CameraFree::Reset(const Vec3 & newpos, const Quat & newquat)
{
	leftright_rotation = 0;
	updown_rotation = 0;
	rotation = newquat;
	position = newpos + offset;
}

void CameraFree::Rotate(float up, float left)
{
	updown_rotation += up;
	if (updown_rotation > 1.0) updown_rotation = 1.0;
	if (updown_rotation <-1.0) updown_rotation =-1.0;
	leftright_rotation += left;

	rotation.LoadIdentity();
	rotation.Rotate(updown_rotation, Direction::Right);
	rotation.Rotate(leftright_rotation, Direction::Up);
}

void CameraFree::Move(float dx, float dy, float dz)
{
	Vec3 move(dx, dy, dz);
	Vec3 forward = Direction::Forward * Direction::Forward.dot(move);
	rotation.RotateVector(forward);
	position = position + forward;
}
