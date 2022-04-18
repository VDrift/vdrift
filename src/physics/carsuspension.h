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

#ifndef _CARSUSPENSION_H
#define _CARSUSPENSION_H

#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "macros.h"
#include "minmax.h"

#include <iosfwd>

class PTree;

inline btVector3 Mix(btVector3 a, btVector3 b, btScalar t)
{
	return a + (b - a) * t;
}

// only correct for small angles between a and b
inline btQuaternion Mix(btQuaternion a, btQuaternion b, btScalar t)
{
	/* large angle correction
	btScalar d = a.dot(b);
	assert(d > 0);
	btScalar k = btScalar(0.931872) + d * (btScalar(-1.25654) + d * btScalar(0.331442));
	t = t + t * (t - btScalar(0.5)) * (t - 1) * k;
	*/
	return (a + (b - a) * t).normalized();
}

struct CarSuspensionInfo
{
	// coilover(const)
	btScalar spring_constant; ///< the suspension spring constant
	btScalar anti_roll; ///< the spring constant for the anti-roll bar
	btScalar bounce; ///< suspension compression damping
	btScalar rebound; ///< suspension decompression damping
	btScalar travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped

	// suspension geometry(const)
	btVector3 position; ///< the position of the wheel when the suspension is fully extended (zero g)
	btScalar steering_angle; ///< maximum steering angle in degrees
	btScalar ackermann; ///< /// for ideal ackemann steering_toe = atan(0.5 * steering_axis_length / axes_distance)
	btScalar camber; ///< camber angle in degrees. sign convention depends on the side
	btScalar caster; ///< caster angle in degrees. sign convention depends on the side
	btScalar toe; ///< toe angle in degrees. sign convention depends on the side

	btQuaternion orientations[5];	// orientation samples [0, 1/3 travel, 2/3 travel, travel]
	btVector3 positions[5];			// position samples

	btScalar inv_mass; ///< 1 / unsprung mass

	CarSuspensionInfo(); ///< default constructor makes an S2000-like car

	void GetWheelTransform(btScalar displacement, btQuaternion & r, btVector3 & p) const
	{
		btScalar n = 4 * Clamp(displacement / travel, btScalar(0), btScalar(1));
		int i = n;
		int i1 = Min(4, i + 1);
		btScalar f = n - i;
		p = Mix(positions[i], positions[i1], f);
		r = Mix(orientations[i], orientations[i1], f);
	}

	btVector3 GetWheelPosition(btScalar displacement) const
	{
		btQuaternion r;
		btVector3 p;
		GetWheelTransform(displacement, r, p);
		return p;
	}
};

class CarSuspension
{
public:
	CarSuspension();

	virtual ~CarSuspension() {}

	btScalar GetAntiRoll() const {return info.anti_roll;}

	btScalar GetStiffness() const {return info.spring_constant;}

	btScalar GetDamping() const {return info.bounce;}

	btScalar GetMaxSteeringAngle() const {return info.steering_angle;}

	/// wheel orientation relative to car
	const btQuaternion & GetWheelOrientation() const {return orientation;}

	/// wheel position relative to car
	const btVector3 & GetWheelPosition() const {return position;}

	btVector3 GetWheelPosition(btScalar displacement) const {return info.GetWheelPosition(displacement);}

	/// wheel overtravel
	btScalar GetOvertravel() const {return overtravel;}

	/// wheel displacement
	btScalar GetDisplacement() const {return displacement;}

	/// steering: -1.0 is maximum right lock and 1.0 is maximum left lock
	void SetSteering(btScalar value);

	/// override current displacement value
	void SetDisplacement(btScalar value);

	/// update wheel position and orientation due to displacement
	/// simulate wheel rebound to limit negative delta
	void UpdateDisplacement(btScalar displacement_delta, btScalar dt);

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "---Suspension---" << "\n";
		out << "Displacement: " << displacement << "\n";
		out << "Steering angle: " << steering_angle * btScalar(180 / M_PI) << "\n";
	}

	template <class Serializer>
	bool Serialize(Serializer & s)
	{
		_SERIALIZE_(s, steering_angle);
		_SERIALIZE_(s, displacement);
		_SERIALIZE_(s, last_displacement);
		return true;
	}

	static bool Load(
		const PTree & cfg_wheel,
		btScalar wheel_mass,
		btScalar wheel_load,
		CarSuspension & suspension,
		std::ostream & error);

protected:
	// constants
	CarSuspensionInfo info;
	btQuaternion orientation_ext;	// toe and camber
	btVector3 steering_axis;
	btScalar tan_ackermann;

	// state
	btQuaternion orientation_steer;
	btQuaternion orientation;
	btVector3 position;
	btScalar steering_angle;

	btScalar overtravel;
	btScalar displacement;
	btScalar last_displacement;
	btScalar wheel_contact;

	void Init(const CarSuspensionInfo & info);
};

#endif
