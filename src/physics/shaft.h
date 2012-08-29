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

#ifndef _SIM_SHAFT_H
#define _SIM_SHAFT_H

#include "LinearMath/btScalar.h"
#include "LinearMath/btMinMax.h"

namespace sim
{

class Shaft
{
public:
	/// default constructor
	Shaft(btScalar inertia = 1E37);

	/// current shaft angle rad
    btScalar getAngle() const;

	/// current shaft velocity rad/s
	btScalar getAngularVelocity() const;

	/// inverse shaft axis inertia
	btScalar getInertiaInv() const;

	/// shaft axis inertia
	btScalar getInertia() const;

	/// init shaft
	void setInertia(btScalar value);

	/// override shaft velocity rad/s
	void setAngularVelocity(btScalar value);

	/// apply angular momentum/impulse to shaft
    void applyImpulse(btScalar impulse);

	/// update shaft rotation angle
	void integrate(btScalar dt);

	/// fixed shaft instance
	static Shaft & getFixed();

private:
    btScalar inertia;
    btScalar inertiaInv;
    btScalar angularVelocity;
    btScalar angle;
};

// implementation

inline Shaft::Shaft(btScalar inertia) :
	inertia(inertia),
	inertiaInv(1 / inertia),
	angularVelocity(0),
	angle(0)
{
	// ctor
}

inline btScalar Shaft::getAngle() const
{
	return angle;
}

inline btScalar Shaft::getAngularVelocity() const
{
	return angularVelocity;
}

inline btScalar Shaft::getInertiaInv() const
{
	return inertiaInv;
}

inline btScalar Shaft::getInertia() const
{
	return inertia;
}

inline void Shaft::setInertia(btScalar value)
{
	inertia = value;
	inertiaInv = 1 / value;
}

inline void Shaft::setAngularVelocity(btScalar value)
{
	angularVelocity = value;
}

inline void Shaft::applyImpulse(btScalar impulse)
{
	angularVelocity += inertiaInv * impulse;
}

inline void Shaft::integrate(btScalar dt)
{
	angle += angularVelocity * dt;
}

inline Shaft & Shaft::getFixed()
{
	static Shaft fixed;
	return fixed;
}

}

#endif
