#ifndef _CARSUSPENSION_H
#define _CARSUSPENSION_H

#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "linearinterp.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class PTree;

struct CARSUSPENSIONINFO
{
	// coilover(const)
	btScalar spring_constant; ///< the suspension spring constant
	btScalar anti_roll; ///< the spring constant for the anti-roll bar
	btScalar bounce; ///< suspension compression damping
	btScalar rebound; ///< suspension decompression damping
	btScalar travel; ///< how far the suspension can travel from the zero-g fully extended position around the hinge arc before wheel travel is stopped
	LINEARINTERP<btScalar> damper_factors;
	LINEARINTERP<btScalar> spring_factors;

	// suspension geometry(const)
	btVector3 position; ///< the position of the wheel when the suspension is fully extended (zero g)
	btScalar steering_angle; ///< maximum steering angle in degrees
	btScalar ackermann; ///< /// for ideal ackemann steering_toe = atan(0.5 * steering_axis_length / axes_distance)
	btScalar camber; ///< camber angle in degrees. sign convention depends on the side
	btScalar caster; ///< caster angle in degrees. sign convention depends on the side
	btScalar toe; ///< toe angle in degrees. sign convention depends on the side

	CARSUSPENSIONINFO(); ///< default constructor makes an S2000-like car
};

class CARSUSPENSION
{
public:
	CARSUSPENSION();

	virtual ~CARSUSPENSION() {}

	const btScalar & GetAntiRoll() const {return info.anti_roll;}

	const btScalar & GetMaxSteeringAngle() const {return info.steering_angle;}

	/// wheel orientation relative to car
	const btQuaternion & GetWheelOrientation() const {return orientation;}

	/// wheel position relative to car
	const btVector3 & GetWheelPosition() const {return position;}

	/// displacement: fraction of suspension travel
	virtual btVector3 GetWheelPosition(btScalar displacement) = 0;

	/// force acting onto wheel
	const btScalar & GetWheelForce() const {return wheel_force;}

	/// suspension force acting onto car body
	const btScalar & GetForce() const {return force;}

	/// relative wheel velocity
	const btScalar & GetVelocity() const {return wheel_velocity;}

	/// wheel overtravel
	const btScalar & GetOvertravel() const {return overtravel;}

	/// wheel displacement
	const btScalar & GetDisplacement() const {return displacement;}

	/// displacement fraction: 0.0 fully extended, 1.0 fully compressed
	btScalar GetDisplacementFraction() const {return displacement / info.travel;}

	/// steering: -1.0 is maximum right lock and 1.0 is maximum left lock
	virtual void SetSteering(const btScalar & value);

	void SetDisplacement ( const btScalar & value );

	btScalar GetForce ( btScalar dt );

	void DebugPrint(std::ostream & out) const;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, steering_angle);
		_SERIALIZE_(s, displacement);
		return true;
	}

	static bool Load(
		const PTree & cfg_wheel,
		CARSUSPENSION *& suspension,
		std::ostream & error);

	friend class joeserialize::Serializer;

protected:
	CARSUSPENSIONINFO info;

	// suspension
	btQuaternion orientation_ext;
	btVector3 steering_axis;
	btQuaternion orientation;
	btVector3 position;
	btScalar steering_angle;
	btScalar spring_force;
	btScalar damp_force;
	btScalar force;

	// wheel
	btScalar overtravel;
	btScalar displacement;
	btScalar last_displacement;
	btScalar wheel_velocity;
	btScalar wheel_force;

	void Init(const CARSUSPENSIONINFO & info);
};

#endif
