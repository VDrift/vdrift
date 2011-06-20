#ifndef _CARBRAKE_H
#define _CARBRAKE_H

#include "LinearMath/btScalar.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CARBRAKE
{
	friend class joeserialize::Serializer;
	private:
		//constants (not actually declared as const because they can be changed after object creation)
		btScalar friction; ///< sliding coefficient of friction for the brake pads on the rotor
		btScalar max_pressure; ///< maximum allowed pressure
		btScalar radius; ///< effective radius of the rotor
		btScalar area; ///< area of the brake pads
		btScalar bias; ///< the fraction of the pressure to be applied to the brake
		btScalar threshold; ///< brake locks when the linear brake velocity divided by the normal force is less than this value
		btScalar handbrake; ///< the friction factor that is applied when the handbrake is pulled.  this is usually 1.0 for rear brakes and 0.0 for front brakes, but could be any number

		//variables
		btScalar brake_factor;
		btScalar handbrake_factor; ///< this is separate so that ABS does not get applied to the handbrake

		//for info only
		btScalar lasttorque;


	public:
		//default constructor makes an S2000-like car
		CARBRAKE() :
			friction(0.73),
			max_pressure(4e6),
			radius(0.14),
			area(0.015),
			threshold(2e-4),
			brake_factor(0),
			handbrake_factor(0),
			lasttorque(0)
		{
			// ctor
		}

		void DebugPrint(std::ostream & out) const
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

		void SetFriction(const btScalar & value)
		{
			friction = value;
		}

		btScalar GetFriction() const
		{
			return friction;
		}

		void SetMaxPressure(const btScalar & value)
		{
			max_pressure = value;
		}

		void SetRadius(const btScalar & value)
		{
			radius = value;
		}

		btScalar GetRadius() const
		{
			return radius;
		}

		void SetArea(const btScalar & value)
		{
			area = value;
		}

		void SetBias(const btScalar & value)
		{
			bias = value;
		}

		btScalar GetBrakeFactor() const
		{
			return brake_factor;
		}

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s, brake_factor);
			_SERIALIZE_(s, handbrake_factor);
			return true;
		}

		void SetHandbrake(const btScalar & value)
		{
			handbrake = value;
		}

		///ranges from 0.0 (no brakes applied) to 1.0 (brakes applied)
		void SetHandbrakeFactor(const btScalar & value)
		{
			handbrake_factor = value;
		}
};

#endif
