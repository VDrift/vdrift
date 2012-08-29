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

#ifndef _SIM_WHEEL_H
#define _SIM_WHEEL_H

#include "suspension.h"
#include "shaft.h"
#include "brake.h"
#include "tire.h"
#include "ray.h"

namespace sim
{

struct WheelContact;
class FractureBody;
class World;

struct WheelInfo
{
	SuspensionInfo suspension;
	BrakeInfo brake;
	TireInfo tire;
	btScalar inertia;
	btScalar radius;
	btScalar width;
	btScalar mass;
};

class Wheel
{
public:
	/// default constructor (use init)
	Wheel();

	/// init before usage
	void init(
		const WheelInfo & info,
		World & world,
		FractureBody & body);

	/// update wheel displacement (ray test)
	/// raylen should be greater than wheel radius
	/// returns false if there is no contact
	bool updateDisplacement(btScalar raylen = 2);

	/// dynamic stiffness delta to account for anti-roll effect
	/// k = antiroll_stiffness_const * wheels_dislacement_delta / displacement
	/// when no anti roll bar installed set to 0
	void setAntiRollStiffness(btScalar value);

	/// update contact, make sure to update displacement first
	/// returns false if there is no contact
	bool updateContact(btScalar dt, WheelContact & contact);

	/// enable/disable abs
	void setABS(bool value);

	/// enable/disable abs
	void setTCS(bool value);

	/// wheel (center) world space position
	const btVector3 & getPosition() const;

	/// wheel radius
	btScalar getRadius() const;

	/// wheel width
	btScalar getWidth() const;

	/// true if abs active
	bool getABS() const;

	/// true if tcs active
	bool getTCS() const;

	/// wheel components
	Suspension suspension;
	Shaft shaft;
	Brake brake;
	Tire tire;
	Ray ray;

private:
	World * world;
	FractureBody * body;
	btTransform transform;
	btScalar angvel;
	btScalar radius;
	btScalar width;
	btScalar mass;
	btScalar antiroll;
	bool has_contact;
	bool abs_enabled;
	bool tcs_enabled;
	bool abs_active;
	bool tcs_active;
};

// implementation

inline void Wheel::setAntiRollStiffness(btScalar value)
{
	antiroll = value;
}

inline void Wheel::setABS(bool value)
{
	abs_enabled = value;
}

inline void Wheel::setTCS(bool value)
{
	tcs_enabled = value;
}

inline const btVector3 & Wheel::getPosition() const
{
	return transform.getOrigin();
}

inline btScalar Wheel::getRadius() const
{
	return radius;
}

inline btScalar Wheel::getWidth() const
{
	return width;
}

inline bool Wheel::getABS() const
{
	return abs_active;
}

inline bool Wheel::getTCS() const
{
	return tcs_active;
}

}

#endif
