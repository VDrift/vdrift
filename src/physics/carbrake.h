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

#ifndef _CARBRAKE_H
#define _CARBRAKE_H

#include "LinearMath/btScalar.h"
#include "minmax.h"
#include "macros.h"

class CarBrake
{
	private:
		//constants (not actually declared as const because they can be changed after object creation)
		btScalar friction; ///< sliding coefficient of friction for the brake pads on the rotor
		btScalar max_pressure; ///< maximum allowed pressure
		btScalar radius; ///< effective radius of the rotor
		btScalar area; ///< area of the brake pads
		btScalar bias; ///< the fraction of the pressure to be applied to the brake
		btScalar handbrake; ///< the fraction of the pressure to be applied when the handbrake is pulled

		//variables
		btScalar brake_factor;
		btScalar handbrake_factor; ///< this is separate so that ABS does not get applied to the handbrake

		//for info only
		btScalar lasttorque;


	public:
		//default constructor makes an S2000-like car
		CarBrake() :
			friction(0.73),
			max_pressure(4e6),
			radius(0.14),
			area(0.015),
			bias(1),
			handbrake(1),
			brake_factor(0),
			handbrake_factor(0),
			lasttorque(0)
		{
			// ctor
		}

		template <class Stream>
		void DebugPrint(Stream & out) const
		{
			out << "---Brake---" << "\n";
			out << "Control: " << brake_factor << ", " << handbrake_factor << "\n";
			out << "Torque: " << lasttorque << "\n";
		}

		///brake_factor ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
		void SetBrakeFactor(btScalar newfactor)
		{
			brake_factor = newfactor;
		}

		btScalar GetMaxTorque() const
		{
			return (max_pressure * area) * (friction * radius);
		}

		///brake torque magnitude
		btScalar GetTorque()
		{
			lasttorque = GetMaxTorque() * (bias * brake_factor + handbrake * handbrake_factor);
			return lasttorque;
		}

		void SetFriction(btScalar value)
		{
			friction = value;
		}

		btScalar GetFriction() const
		{
			return friction;
		}

		void SetMaxPressure(btScalar value)
		{
			max_pressure = value;
		}

		void SetRadius(btScalar value)
		{
			radius = value;
		}

		btScalar GetRadius() const
		{
			return radius;
		}

		void SetArea(btScalar value)
		{
			area = value;
		}

		void SetBias(btScalar value)
		{
			bias = value;
		}

		btScalar GetBrakeFactor() const
		{
			return brake_factor;
		}

		void SetHandbrake(btScalar value)
		{
			handbrake = value;
		}

		///ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
		void SetHandbrakeFactor(btScalar value)
		{
			handbrake_factor = value;
		}

		template <class Serializer>
		bool Serialize(Serializer & s)
		{
			_SERIALIZE_(s, brake_factor);
			_SERIALIZE_(s, handbrake_factor);
			return true;
		}
};

#endif
