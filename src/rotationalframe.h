#ifndef _ROTATIONALFRAME_H
#define _ROTATIONALFRAME_H

#include "quaternion.h"
#include "mathvector.h"
#include "matrix3.h"
#include "joeserialize.h"
#include "macros.h"

//#define NSV
//#define VELOCITYVERLET
#define SUVAT
//#define EULER
//#define SECOND_ORDER	// Samuel R. Buss second order angular momentum(and energy) preserving integrator

template <typename T>
class ROTATIONALFRAME
{
public:
	ROTATIONALFRAME() :
		torque(0),
		halfstep(false)
	{
	}

	const MATHVECTOR <T, 3> & GetTorque() const
	{
		return torque;
	}

	/// get torque needed to reach a new angular velocity
	const MATHVECTOR <T, 3> GetTorque(const MATHVECTOR<T, 3> & angvel_new, const T dt) const
	{
		MATHVECTOR<T, 3> angvel_delta = angvel_new - angular_velocity;
		MATHVECTOR<T, 3> angmom_delta = world_inertia_tensor.Multiply(angvel_delta);
		return angmom_delta / dt;
	}

	const MATHVECTOR <T, 3> GetAngularVelocity() const
	{
		return angular_velocity;
	}

	const QUATERNION <T> & GetOrientation() const
	{
		return orientation;
	}

	const MATRIX3 <T> & GetInertia() const
	{
		return inertia_tensor;
	}

	const MATRIX3 <T> & GetWorldInertia() const
	{
		return world_inertia_tensor;
	}

	const MATRIX3 <T> & GetInvWorldInertia() const
	{
		return world_inverse_inertia_tensor;
	}

	///this must only be called between integrate1 and integrate2 steps
	void ApplyTorque(const MATHVECTOR <T, 3> & t)
	{
		assert(halfstep);
		torque = torque + t;
	}

	void SetTorque(const MATHVECTOR <T, 3> & t)
	{
		assert(halfstep);
		torque = t;
	}

	void SetAngularVelocity(const MATHVECTOR <T, 3> & newangvel)
	{
		angular_momentum = world_inertia_tensor.Multiply(newangvel);
		angular_velocity = newangvel;
	}

	void SetOrientation(const QUATERNION <T> & neworient)
	{
		orientation = neworient;
	}

	void SetInertia(const MATRIX3 <T> & inertia)
	{
		// update inertia tensors
		inertia_tensor = inertia;
		inverse_inertia_tensor = inertia_tensor.Inverse();
		world_inverse_inertia_tensor = orientmat.Transpose().Multiply(inverse_inertia_tensor).Multiply(orientmat);
		world_inertia_tensor = orientmat.Transpose().Multiply(inertia_tensor).Multiply(orientmat);

		// update angular velocity
		angular_velocity = GetAngularVelocityFromMomentum(angular_momentum);
		angular_momentum = world_inertia_tensor.Multiply(angular_velocity);
		angular_velocity = GetAngularVelocityFromMomentum(angular_momentum);
	}

	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces can only be set between steps 1 and 2
	void Integrate1(const T & dt)
	{
		assert(!halfstep);

#ifdef VELOCITYVERLET
		angular_momentum = angular_momentum + torque * dt * 0.5;
		orientation = orientation + GetSpinFromMomentum(angular_momentum) * dt;
		orientation.Normalize();
		RecalculateSecondary();
#endif

		torque.Set(0.0);
		halfstep = true;
	}

	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces must be set between steps 1 and 2
	void Integrate2(const T & dt)
	{
		assert(halfstep);

#ifdef NSV
		angular_momentum = angular_momentum + torque * dt;
		orientation = orientation + GetSpinFromMomentum(angular_momentum) * dt;
		orientation.Normalize();
#endif

#ifdef EULER
		orientation = orientation + GetSpinFromMomentum(angular_momentum) * dt;
		orientation.Normalize();
		angular_momentum = angular_momentum + torque * dt;
#endif

#ifdef SUVAT
		orientation = orientation + GetSpinFromMomentum(angular_momentum + torque * dt * 0.5) * dt;
		orientation.Normalize();
		angular_momentum = angular_momentum + torque * dt;
#endif

#ifdef SECOND_ORDER
		MATHVECTOR<T, 3> ang_acc =
			world_inverse_inertia_tensor.Multiply(torque - angular_velocity.cross(angular_momentum));
		MATHVECTOR<T, 3> avg_rot =
			angular_velocity + ang_acc * dt / 2.0 + ang_acc.cross(angular_velocity) * dt * dt / 12.0;
		QUATERNION<T> dq =
			QUATERNION<T>(avg_rot[0], avg_rot[1], avg_rot[2], 0) * orientation * 0.5 * dt;
		orientation = orientation + dq;
		orientation.Normalize();
#endif

#ifdef VELOCITYVERLET
		angular_momentum = angular_momentum + torque * dt * 0.5;
#else
		RecalculateSecondary();
#endif

		halfstep = false;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, orientation);
		_SERIALIZE_(s, angular_momentum);
		_SERIALIZE_(s, torque);
		return true;
	}

private:
	//primary
	QUATERNION <T> orientation;
	MATHVECTOR <T, 3> angular_momentum;
	MATHVECTOR <T, 3> torque;

	//secondary
	MATRIX3 <T> orientmat; ///< 3x3 orientation matrix generated during inertia tensor rotation to worldspace and cached here
	MATRIX3 <T> world_inverse_inertia_tensor; ///< inverse inertia tensor in worldspace, cached here
	MATRIX3 <T> world_inertia_tensor;
	MATHVECTOR <T, 3> angular_velocity; ///< calculated from angular_momentum, cached here

	//constants
	MATRIX3 <T> inverse_inertia_tensor; //used for calculations every frame
	MATRIX3 <T> inertia_tensor; //used for the GetInertia function only

	//housekeeping
	bool halfstep;

	void RecalculateSecondary()
	{
		angular_velocity = GetAngularVelocityFromMomentum(angular_momentum);
		orientation.GetMatrix3(orientmat);
		world_inverse_inertia_tensor = orientmat.Transpose().Multiply(inverse_inertia_tensor).Multiply(orientmat);
		world_inertia_tensor = orientmat.Transpose().Multiply(inertia_tensor).Multiply(orientmat);
	}

	///this call depends on having orientmat and world_inverse_inertia_tensor calculated
	MATHVECTOR <T, 3> GetAngularVelocityFromMomentum(const MATHVECTOR <T, 3> & moment) const
	{
		return world_inverse_inertia_tensor.Multiply(moment);
	}

	QUATERNION <T> GetSpinFromMomentum(const MATHVECTOR <T, 3> & ang_moment) const
	{
		const MATHVECTOR <T, 3> ang_vel = GetAngularVelocityFromMomentum(ang_moment);
		QUATERNION <T> qav = QUATERNION <T> (ang_vel[0], ang_vel[1], ang_vel[2], 0);
		return (qav * orientation) * 0.5;
	}
};

#endif
