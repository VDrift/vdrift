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

#include "camera_chase.h"

CAMERA_CHASE::CAMERA_CHASE(const std::string & name) :
	CAMERA(name),
	offset(direction::Up * 2 - direction::Forward * 6),
	posblend_on(true)
{
	rotation.LoadIdentity();
}

void CAMERA_CHASE::SetOffset(const MATHVECTOR <float, 3> & value)
{
	offset = value;
	if (offset.dot(direction::Forward) < 0.001)
	{
		offset = offset - direction::Forward;
	}
}

void CAMERA_CHASE::Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing)
{
	focus = newfocus;
	rotation = focus_facing;
	MATHVECTOR <float, 3> view_offset = offset;
	rotation.RotateVector(view_offset);
	position = focus + view_offset;
}

void CAMERA_CHASE::Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, float dt)
{
	focus = newfocus;
	MATHVECTOR <float, 3> view_offset = offset;
	focus_facing.RotateVector(view_offset);
	MATHVECTOR <float, 3> target_position = focus + view_offset;

	float posblend = 10.0 * dt;
	if (posblend > 1.0) posblend = 1.0;
	if (!posblend_on) posblend = 1.0;
	position = position * (1.0 - posblend) + target_position * posblend;
	rotation = LookAt(position, focus, direction::Up);
}
