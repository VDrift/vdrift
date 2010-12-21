#include "camera_mount.h"

CAMERA_MOUNT::CAMERA_MOUNT(const std::string & name) :
	CAMERA(name),
	effect(0.0),
	stiffness(0.0),
	offset_effect_strength(1.0)
{
	rotation.LoadIdentity();
}

void CAMERA_MOUNT::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	float mass = 1.0;
	float length = 0.05;
	body.SetMass(mass);
	MATRIX3 <float> rotinertia;
	rotinertia.Scale(mass*length*length/3.0);
	body.SetInertia(rotinertia);
	body.SetPosition(MATHVECTOR <float, 3>(0));
	
	MATHVECTOR <float, 3> pos = offset;
	newquat.RotateVector(pos);
	anchor = pos + newpos;
	
	body.SetOrientation(newquat);
	
	UpdatePosition();
}

void CAMERA_MOUNT::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt)
{
	MATHVECTOR <float, 3> pos = offset;
	newdir.RotateVector(pos);
	pos = pos + newpos;
	
	QUATERNION<float> dir = newdir * rotation;
	MATHVECTOR <float, 3> vel = pos - anchor;
	anchor = pos;
	
	effect = (vel.Magnitude()-.02)/0.04;
	if (effect < 0) effect = 0;
	else if (effect > 1) effect = 1;
	//std::cout << vel.Magnitude() << std::endl;
	
	float bumpdiff = randgen.Get();
	float power = pow(bumpdiff, 32);
	if (power < 0) power = 0;
	else if (power > 0.2) power = 0.2;
	float veleffect = std::min(pow(vel.Magnitude()*(2.0-stiffness),3.0),1.0);
	float bumpimpulse = power*130.0*veleffect;
	
	body.Integrate1(dt);

	MATHVECTOR <float, 3> accellocal;// = -accel;
	//(-dir).RotateVector(accellocal);

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
	
	UpdatePosition();
}

void CAMERA_MOUNT::UpdatePosition()
{
	MATHVECTOR <float, 3> modbody = body.GetPosition();
	body.GetOrientation().RotateVector(modbody);
	MATHVECTOR <float, 3> maxallowed(0.05,0.05,0.05);
	maxallowed = maxallowed * 1.0/(stiffness+1.0);
	for (int i = 0; i < 3; i++)
	{
		if (modbody[i] > maxallowed[i]) modbody[i] = maxallowed[i];
		if (modbody[i] < -maxallowed[i]) modbody[i] = -maxallowed[i];
	}
	modbody[1] *= 0.5;
	
	position = anchor + (modbody * effect) * offset_effect_strength;	
}
