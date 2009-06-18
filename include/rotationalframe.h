#ifndef _ROTATIONALFRAME_H
#define _ROTATIONALFRAME_H

#include "quaternion.h"
#include "mathvector.h"
#include "matrix3.h"
#include "joeserialize.h"
#include "macros.h"

//#define NSV
//#define MODIFIEDVERLET
#define SUVAT
//#define EULER

template <typename T>
class ROTATIONALFRAME
{
private:
	//primary
	QUATERNION <T> orientation;
	MATHVECTOR <T, 3> angular_momentum;
	MATHVECTOR <T, 3> torque;
	
	//secondary
	MATHVECTOR <T, 3> old_torque; //this is only state information for the verlet-like integrators
	MATRIX3 <T> orientmat; ///< 3x3 orientation matrix generated during inertia tensor rotation to worldspace and cached here
	MATRIX3 <T> world_inverse_inertia_tensor; ///< inverse inertia tensor in worldspace, cached here
	MATHVECTOR <T, 3> angular_velocity; ///< calculated from angular_momentum, cached here
	
	//constants
	//T inverse_inertia;
	MATRIX3 <T> inverse_inertia_tensor; //used for calculations every frame
	MATRIX3 <T> inertia_tensor; //used for the GetInertia function only
	
	//housekeeping
	bool have_old_torque;
	int integration_step;
	
	void RecalculateSecondary()
	{
		old_torque = torque;
		have_old_torque = true;
		orientation.GetMatrix3(orientmat);
		world_inverse_inertia_tensor = orientmat.Transpose().Multiply(inverse_inertia_tensor).Multiply(orientmat);
		angular_velocity = GetAngularVelocityFromMomentum(angular_momentum);
	}
	
	///this call depends on having orientmat and world_inverse_inertia_tensor calculated
	MATHVECTOR <T, 3> GetAngularVelocityFromMomentum(const MATHVECTOR <T, 3> & moment) const
	{
		//return moment * inverse_inertia;
		//return inverse_inertia_tensor.Multiply(moment);
		//return moment * 1./1000.0;
		
		/*//transform the moment to local body space
		MATHVECTOR <T, 3> bodymoment;
		(-orientation).RotateVector(bodymoment);
		//do the inverse tensor multiply in body space
		MATHVECTOR <T, 3> angvel = inverse_inertia_tensor.Multiply(bodymoment);
		//now transform the result back to world space
		orientation.RotateVector(angvel);
		return angvel;*/
		
		//transform the inertia tensor to world space
		//orientation.GetMatrix3(orientmat);
		//world_inverse_inertia_tensor = orientmat.Transpose().Multiply(inverse_inertia_tensor).Multiply(orientmat);
		return world_inverse_inertia_tensor.Multiply(moment);
	}
	
	QUATERNION <T> GetSpinFromMomentum(const MATHVECTOR <T, 3> & ang_moment) const
	{
		const MATHVECTOR <T, 3> ang_vel = GetAngularVelocityFromMomentum(ang_moment);
		QUATERNION <T> qav = QUATERNION <T> (ang_vel[0], ang_vel[1], ang_vel[2], 0);
		return (qav * orientation) * 0.5;
	}
	
	const MATHVECTOR <T, 3> TransformTorqueToLocal(const MATHVECTOR <T, 3> & sometorque)
	{
		MATHVECTOR <T, 3> outputvec = sometorque;
		//(-orientation).RotateVector(outputvec);
		return outputvec;
	}
	
public:
	ROTATIONALFRAME() : have_old_torque(false),integration_step(0) {}
	
	void SetInertia(const MATRIX3 <T> & inertia)
	{
		inertia_tensor = inertia;
		inverse_inertia_tensor = inertia.Inverse();
	}
	
	const MATRIX3 <T> GetInertia()
	{
		return inertia_tensor;
	}
	
	void SetOrientation(const QUATERNION <T> & neworient)
	{
		orientation = neworient;
	}
	
	void SetAngularVelocity(const MATHVECTOR <T, 3> & newangvel)
	{
		angular_momentum = inverse_inertia_tensor.Inverse().Multiply(newangvel);
		RecalculateSecondary();
	}
	
	const MATHVECTOR <T, 3> GetAngularVelocity() const
	{
		//RecalculateSecondary();
		return angular_velocity;
		//return GetAngularVelocityFromMomentum(angular_momentum);
	}
	
	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces can only be set between steps 1 and 2
	void Integrate1(const T & dt)
	{
		assert(integration_step == 0);
		
		assert (have_old_torque); //you'll get an assert problem unless you call SetInitialTorque at the beginning of the simulation
		
#ifdef MODIFIEDVERLET
		//orientation = orientation + GetSpinFromMomentum(angular_momentum)*dt + GetSpinFromMomentum(old_torque)*dt*dt*0.5;
		orientation = orientation + GetSpinFromMomentum(angular_momentum + old_torque*dt*0.5)*dt;
		orientation.Normalize();
		angular_momentum = angular_momentum + old_torque * dt * 0.5;
		RecalculateSecondary();
#endif
		
		integration_step++;
	}
	
	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces must be set between steps 1 and 2
	void Integrate2(const T & dt)
	{
		assert(integration_step == 2);
#ifdef MODIFIEDVERLET
		angular_momentum = angular_momentum + torque * dt * 0.5;
#endif
		
#ifdef NSV
		//simple NSV integration
		angular_momentum = angular_momentum + torque * dt;
		orientation = orientation + GetSpinFromMomentum(angular_momentum)*dt;
		orientation.Normalize();
#endif
		
#ifdef EULER
		orientation = orientation + GetSpinFromMomentum(angular_momentum)*dt;
		orientation.Normalize();
		angular_momentum = angular_momentum + torque * dt;
#endif
		
#ifdef SUVAT
		//orientation = orientation + GetSpinFromMomentum(angular_momentum)*dt + GetSpinFromMomentum(torque)*dt*dt*0.5;
		orientation = orientation + GetSpinFromMomentum(angular_momentum + torque*dt*0.5)*dt;
		orientation.Normalize();
		angular_momentum = angular_momentum + torque * dt;
#endif
		
		RecalculateSecondary();
		
		integration_step = 0;
	}
	
	///this must only be called between integrate1 and integrate2 steps
	void SetTorque(const MATHVECTOR <T, 3> & newtorque)
	{
		assert(integration_step == 1);
		
		torque = TransformTorqueToLocal(newtorque);
		
		integration_step++;
	}
	
	const MATHVECTOR <T, 3> & GetTorque() {return torque;}
	
	///this must be called once at sim start to set the initial torque present
	void SetInitialTorque(const MATHVECTOR <T, 3> & newtorque)
	{
		assert(integration_step == 0);
		
		old_torque = TransformTorqueToLocal(newtorque);
		have_old_torque = true;
	}

	const QUATERNION <T> & GetOrientation() const
	{
		return orientation;
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,orientation);
		_SERIALIZE_(s,angular_momentum);
		_SERIALIZE_(s,torque);
		RecalculateSecondary();
		return true;
	}
};

#endif
