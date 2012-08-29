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

#ifndef _SIM_SUSPENSION_H
#define _SIM_SUSPENSION_H

#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"

namespace sim
{

struct SuspensionArm
{
	btVector3 anchor;	///< rotation axis position relative to chassis
	btVector3 axis;		///< rotation axis normalized
	btVector3 dir;		///< arm direction: (hub - anchor).normalized()
	btScalar length;	///< arm length: (hub - anchor).length()
};

struct SuspensionInfo
{
	SuspensionArm upper_arm;	///< used for double wishbone, macpherson
	SuspensionArm lower_arm;	///< required used for all supported types
	btVector3 steering_axis;	///< steering axis relative to wheel hub, calculated from caster, camber, toe
	btQuaternion orientation0;	///< wheel rotation relative to wheel hub, calculated from caster, camber, toe
	btVector3 position0;		///< position of the wheel hub when the suspension is fully extended (zero g)
	btScalar stiffness;			///< suspension spring constant
	btScalar bounce;			///< suspension compression damping
	btScalar rebound;			///< suspension decompression damping
	btScalar travel;			///< max suspension travel from the fully extended position around the hinge arc
	btScalar max_steering_angle;///< maximum steering angle in degrees
	btScalar ackermann;			///< ackermann angle, ideal = atan(0.5 * steering_axis_length / axes_distance)
	SuspensionInfo();			///< default constructor makes an S2000-like car
};

class Suspension : private SuspensionInfo
{
public:
	Suspension();

	void init(const SuspensionInfo & info);

	/// steering: -1.0 is maximum right lock and 1.0 is maximum left lock
	void setSteering(btScalar value);

	/// update position, orienatation and stifness, damping values
	/// from steering angle and displacement
	void setDisplacement(btScalar value);

	/// maximum supported steering angle
	const btScalar & getMaxSteeringAngle() const;

	/// wheel orientation relative to car
	const btQuaternion & getOrientation() const;

	/// wheel position relative to car
	const btVector3 & getPosition() const;

	/// current suspension displacement
	btScalar getDisplacement() const;

	/// current susension overtravel
	btScalar getOvertravel() const;

	/// current suspension stiffness coefficient
	btScalar getStiffness() const;

	/// current suspension damping coefficient
	btScalar getDamping() const;

private:
	btQuaternion hub_orientation;	// local hub orientation, relative to upright
	btQuaternion orientation;		// wheel orientation
	btVector3 position;				// wheel position
	btScalar steering_angle;
	btScalar displacement;
	btScalar overtravel;
	btScalar damping;
};

// implementation

inline const btScalar & Suspension::getMaxSteeringAngle() const
{
	return max_steering_angle;
}

inline const btQuaternion & Suspension::getOrientation() const
{
	return orientation;
}

inline const btVector3 & Suspension::getPosition() const
{
	return position;
}

inline btScalar Suspension::getDisplacement() const
{
	return displacement;
}

inline btScalar Suspension::getOvertravel() const
{
	return overtravel;
}

inline btScalar Suspension::getStiffness() const
{
	return stiffness;
}

inline btScalar Suspension::getDamping() const
{
	return damping;
}

}

#endif
