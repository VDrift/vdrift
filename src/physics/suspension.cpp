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

#include "suspension.h"

namespace sim
{

SuspensionInfo::SuspensionInfo() :
	steering_axis(0, 0, 1),
	orientation0(btQuaternion::getIdentity()),
	position0(0, 0, 0),
	stiffness(50000),
	bounce(2500),
	rebound(4000),
	travel(0.2),
	max_steering_angle(0),
	ackermann(0)
{
	// ctor
}

void Suspension::init(const SuspensionInfo & info)
{
	SuspensionInfo::operator=(info);
	position = info.position0;
	damping = info.bounce;
}

Suspension::Suspension() :
	hub_orientation(btQuaternion::getIdentity()),
	orientation(btQuaternion::getIdentity()),
	position(0, 0, 0),
	steering_angle(0),
	displacement(0),
	damping(0)
{
	// ctor
}

void Suspension::setSteering(btScalar value)
{
	btScalar alpha = -value * max_steering_angle * M_PI / 180.0;
	steering_angle = 0.0;
	if (alpha != 0.0)
	{
		steering_angle = atan(1.0 / (1.0 / tan(alpha) - tan(ackermann * M_PI / 180.0)));
	}
	btQuaternion steer_rotation(steering_axis, steering_angle);
	hub_orientation = steer_rotation * orientation0;
}

void Suspension::setDisplacement(btScalar value)
{
	btScalar delta = value - displacement;
	damping = (delta > 0) ? bounce : rebound;
	displacement = value;
	btClamp(displacement, btScalar(0), travel);

	btVector3 up(0, 0, 1);
	btVector3 old_dir = lower_arm.dir * lower_arm.length; // constant
	btVector3 hub_offset = position0 - lower_arm.anchor - old_dir; // constant
	btVector3 new_dir = old_dir + up * displacement;
	new_dir.normalize();
	position = lower_arm.anchor + new_dir * lower_arm.length + hub_offset;
	orientation = hub_orientation;
}

}
