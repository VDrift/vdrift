#include "camera.h"
#include "signalprocessing.h"

#include <cmath>

using std::endl;

void CAMERA_CHASE::Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, const MATHVECTOR <float, 3> & accel, float dt)
{
	focus = newfocus;
	MATHVECTOR <float, 3> view_offset(-chase_distance, 0, chase_height);
	focus_facing.RotateVector(view_offset);
	MATHVECTOR <float, 3> target_position = focus + view_offset;
	float posblend = 10.0 * dt;
	//if (lookbehind)
	//	posblend *= 10;
	
	if (posblend > 1.0)
		posblend = 1.0;
	if (!posblend_on)
		posblend = 1.0;
	position = position * (1.0 - posblend) + target_position * posblend;

	MATHVECTOR <float, 3> focus_offset(0, 0, 0);
	//if (lookbehind)
	//{
	//	focus_offset[2] = 1.0;
	//}
	//else 
	if (chase_distance < 0.0001)
	{
		focus_offset.Set(1.0, 0.0, 0.0);
		focus_facing.RotateVector(focus_offset);
	}

	LookAt(position, focus + focus_offset, MATHVECTOR <float, 3> (0, 0, 1));
}

void CAMERA_CHASE::LookAt(MATHVECTOR <float, 3> eye, MATHVECTOR <float, 3> center, MATHVECTOR <float, 3> up)
{
	MATHVECTOR <float, 3> forward(center - eye);
	forward = forward.Normalize();
	MATHVECTOR <float, 3> side = (forward.cross(up)).Normalize();
	MATHVECTOR <float, 3> realup = side.cross(forward);

	//rotate so the camera is pointing along the forward line
	MATHVECTOR <float, 3> curforward (1,0,0);
	float theta = AngleBetween(forward, curforward);
	assert(theta == theta);
	orientation.LoadIdentity();
	MATHVECTOR <float, 3> axis = forward.cross(curforward).Normalize();
	orientation.Rotate(-theta, axis[0], axis[1], axis[2]);

	//now rotate the camera so it's pointing up
	MATHVECTOR <float, 3> curup (0,0,1);
	orientation.RotateVector(curup);
	float rollangle = AngleBetween(realup, curup);
	float pi = 3.141593;
	if (curup.dot(side) > 0.0)
		rollangle = (pi - rollangle) + pi;
	axis = forward;
	orientation.Rotate(rollangle, axis[0], axis[1], axis[2]);

	assert(rollangle == rollangle);
}

float CAMERA_SIMPLEMOUNT::Random()
{
	return randgen.Get();
}

float CAMERA_SIMPLEMOUNT::Distribution(float input)
{
	float sign = 1.0;
	if (input < 0)
		sign = -1.0;
	float power = pow(std::abs(input),64.0);
	power -= 0.2;
	if (power < 0)
		power = 0;
    assert(power <= 1.0f);
	return power * sign * 8.0;
}

void CAMERA_SIMPLEMOUNT::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> & accel, float dt)
{
	MATHVECTOR <float, 3> vel = newpos - position;

	MATHVECTOR <float, 3> randvec;
	randvec[1] = Distribution(Random())*0.5;
	randvec[2] = Distribution(Random());

	offset_randomwalk = offset_randomwalk + randvec*dt;

	MATHVECTOR <float, 3> controlleroutput;
	for (int i = 0; i < 3; i++)
	{
		float error = -offsetdelay[i].Process(offsetdelay[i].Process(offset_filtered[i]));
		controlleroutput[i] = offsetpid[i].Process(error, 0);
	}

	//std::cout << offset_filtered << " || " << controlleroutput << std::endl;

	for (int i = 0; i < 3; i++)
		offset_filtered[i] = offsetlowpass[i].Process(offset_randomwalk[i]+controlleroutput[i]);

	float effect = vel.Magnitude();
	effect -= 0.04;
	effect /= 0.2;
	if (effect < 0)
		effect = 0;
	if (effect > 1.0)
		effect = 1.0;
	effect *= effect;
	//std::cout << effect << std::endl;

	offset_final = offset_filtered*effect;

	position = newpos + offset_final;
	orientation = orientation.QuatSlerp(newdir, dt*40.0);
}

void CAMERA_SIMPLEMOUNT::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	offsetdelay.clear();
	offsetlowpass.clear();
	offsetpid.clear();
	for (int i = 0; i < 3; i++)
	{
		offsetdelay.push_back(signalprocessing::DELAY(3-2*stiffness));
		offsetlowpass.push_back(signalprocessing::LOWPASS(.1*(stiffness+1.0)));
		offsetpid.push_back(signalprocessing::PID(0.2*stiffness,0.25*(stiffness+1.0),0,false));
	}

	position = newpos;
	orientation = newquat;
	offset_randomwalk.Set(0,0,0);
}

void CAMERA_MOUNT::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	float mass = 1.0;
	float length = 0.05;
	body.SetMass(mass);
	MATRIX3 <float> rotinertia;
	rotinertia.Scale(mass*length*length/3.0);
	body.SetInertia(rotinertia);
	body.SetInitialForce(MATHVECTOR <float, 3> (0));
	body.SetInitialTorque(MATHVECTOR <float, 3> (0));
	body.SetPosition(MATHVECTOR <float, 3>(0));
	
	MATHVECTOR <float, 3> pos = offset;
	newquat.RotateVector(pos);
	anchor = pos + newpos;
	
	body.SetOrientation(newquat);
}

void CAMERA_MOUNT::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> & accel, float dt)
{
	MATHVECTOR <float, 3> pos = offset;
	newdir.RotateVector(pos);
	pos = pos + newpos;
	QUATERNION<float> dir = newdir * rotation;
	
	MATHVECTOR <float, 3> vel = pos - anchor;
	
	anchor = pos;
	
	effect = (vel.Magnitude()-.02)/0.04;
	if (effect < 0)
		effect = 0;
	if (effect > 1)
		effect = 1;
	//std::cout << vel.Magnitude() << std::endl;
	
	float bumpdiff = randgen.Get();
	float power = pow(bumpdiff, 32.0);
	if (power < 0)
		power = 0;
	assert(power <= 1.0f);
	if (power > 0.2)
		power = 0.2;
	float veleffect = std::min(pow(vel.Magnitude()*(2.0-stiffness),3.0),1.0);
	float bumpimpulse = power*130.0*veleffect;
	
	body.Integrate1(dt);

	MATHVECTOR <float, 3> accellocal = -accel;
	(-dir).RotateVector(accellocal);

	float k = 800.0+stiffness*800.0*4.0;
	float c = 2.0*std::sqrt(k*body.GetMass())*0.35;
	MATHVECTOR <float, 3> x = body.GetPosition();
	MATHVECTOR <float, 3> bumpforce = MATHVECTOR <float, 3>(0,0,bumpimpulse);
	MATHVECTOR <float, 3> accelforce = accellocal*body.GetMass();
	MATHVECTOR <float, 3> springforce = -x*k;
	MATHVECTOR <float, 3> damperforce = -body.GetVelocity()*c;
	body.SetForce(springforce+damperforce+accelforce+bumpforce);
	
	//std::cout << bumpdiff << ", " << power << ", " << vel.Magnitude() << ", " << veleffect << ", " << bumpimpulse << std::endl;
	//std::cout << accellocal << std::endl;
	
	body.SetTorque(MATHVECTOR <float, 3> (0));
	body.Integrate2(dt);
	
	body.SetOrientation(dir);
}

MATHVECTOR <float, 3> CAMERA_MOUNT::GetPosition() const
{
	MATHVECTOR <float, 3> modbody = body.GetPosition();
	body.GetOrientation().RotateVector(modbody);
	MATHVECTOR <float, 3> maxallowed(0.05,0.05,0.05);
	maxallowed = maxallowed * 1.0/(stiffness+1.0);
	for (int i = 0; i < 3; i++)
	{
		if (modbody[i] > maxallowed[i])
			modbody[i] = maxallowed[i];
		if (modbody[i] < -maxallowed[i])
			modbody[i] = -maxallowed[i];
	}
	modbody[1] *= 0.5;

	return anchor+(modbody*effect)*offset_effect_strength;
}
