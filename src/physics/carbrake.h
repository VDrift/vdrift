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
		btScalar handbrake; ///< the friction factor that is applied when the handbrake is pulled.  this is usually 1.0 for rear brakes and 0.0 for front brakes, but could be any number

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
			out << "Brake control: " << brake_factor << ", " << handbrake_factor << "\n";
			out << "Braking torque: " << lasttorque << "\n";
		}

		///brake_factor ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
		void SetBrakeFactor(btScalar newfactor)
		{
			brake_factor = newfactor;
		}

		///brake torque magnitude
		btScalar GetTorque()
		{
			assert(handbrake_factor != -1);
			btScalar brake = brake_factor > handbrake * handbrake_factor ? brake_factor : handbrake * handbrake_factor;
			btScalar pressure = brake * bias * max_pressure;
			btScalar normal = pressure * area;
			btScalar torque = friction * normal * radius;
			lasttorque = torque;
			return torque;
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
