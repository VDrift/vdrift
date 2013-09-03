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

CameraChase::CameraChase(const std::string & name) :
	Camera(name),
	offset(Direction::Up * 2 - Direction::Forward * 6),
	posblend_on(true)
{
	rotation.LoadIdentity();
}

void CameraChase::SetOffset(const Vec3 & value)
{
	offset = value;
	if (offset.dot(Direction::Forward) < 0.001)
	{
		offset = offset - Direction::Forward;
	}
}

void CameraChase::Reset(const Vec3 & newfocus, const Quat & focus_facing)
{
	focus = newfocus;
	rotation = focus_facing;
	Vec3 view_offset = offset;
	rotation.RotateVector(view_offset);
	position = focus + view_offset;
}

void CameraChase::Update(const Vec3 & newfocus, const Quat & focus_facing, float dt)
{
	focus = newfocus;
	Vec3 view_offset = offset;
	focus_facing.RotateVector(view_offset);
	Vec3 target_position = focus + view_offset;

	float posblend = 10.0 * dt;
	if (posblend > 1.0) posblend = 1.0;
	if (!posblend_on) posblend = 1.0;
	position = position * (1.0 - posblend) + target_position * posblend;
	rotation = LookAt(position, focus, Direction::Up);
}
