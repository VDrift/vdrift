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

class btCollisionWorld;

namespace sim
{

struct WheelContact;
class FractureBody;

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
		btCollisionWorld & world,
		FractureBody & body);

	/// wheel ray test
	/// returns false if there is no contact
	bool updateContact(btScalar raylen = 2);

	/// update contact and setup wheel constraint
	/// returns false if there is no contact
	bool update(btScalar dt, WheelContact & contact);

	/// enable/disable abs
	void setABS(bool value);

	/// enable/disable abs
	void setTCS(bool value);

	/// wheel (center) world space position
	const btVector3 &  getPosition() const;

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
	btCollisionWorld * world;
	FractureBody * body;
	btTransform transform;
	btScalar angvel;
	btScalar radius;
	btScalar width;
	btScalar mass;
	bool abs_enabled;
	bool tcs_enabled;
	bool abs_active;
	bool tcs_active;
};

// implementation

inline void Wheel::setABS(bool value)
{
	abs_enabled = value;
}

inline void Wheel::setTCS(bool value)
{
	tcs_enabled = value;
}

inline const btVector3 &  Wheel::getPosition() const
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
