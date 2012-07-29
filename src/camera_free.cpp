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

CAMERA_FREE::CAMERA_FREE(const std::string & name) :
	CAMERA(name),
	offset(direction::Up * 2 - direction::Forward * 8),
	leftright_rotation(0),
	updown_rotation(0)
{
	rotation.LoadIdentity();
}

void CAMERA_FREE::SetOffset(const MATHVECTOR <float, 3> & value)
{
	offset = value;
	if (offset.dot(direction::Forward) < 0.001)
	{
		offset = offset - direction::Forward;
	}
}

void CAMERA_FREE::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	leftright_rotation = 0;
	updown_rotation = 0;
	rotation = newquat;
	position = newpos + offset;
}

void CAMERA_FREE::Rotate(float up, float left)
{
	updown_rotation += up;
	if (updown_rotation > 1.0) updown_rotation = 1.0;
	if (updown_rotation <-1.0) updown_rotation =-1.0;
	leftright_rotation += left;

	rotation.LoadIdentity();
	rotation.Rotate(updown_rotation, direction::Right);
	rotation.Rotate(leftright_rotation, direction::Up);
}

void CAMERA_FREE::Move(float dx, float dy, float dz)
{
	MATHVECTOR <float, 3> move(dx, dy, dz);
	MATHVECTOR <float, 3> forward = direction::Forward * direction::Forward.dot(move);
	rotation.RotateVector(forward);
	position = position + forward;
}
