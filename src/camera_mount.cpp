#include "camera_mount.h"
#include "coordinatesystem.h"

CAMERA_MOUNT::CAMERA_MOUNT(const std::string & name) :
	CAMERA(name),
	stiffness(0),
	mass(1),
	offset_effect_strength(1),
	effect(0)
{
	rotation.LoadIdentity();
}

void CAMERA_MOUNT::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	MATHVECTOR <float, 3> pos = offset;
	newquat.RotateVector(pos);
	rotation = newquat * offsetrot;
	displacement.Set(0, 0, 0);
	velocity.Set(0, 0, 0);
	UpdatePosition(pos + newpos);
}

void CAMERA_MOUNT::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt)
{
	rotation = newdir * offsetrot;

	MATHVECTOR <float, 3> pos = offset;
	newdir.RotateVector(pos);
	pos = pos + newpos;

	MATHVECTOR <float, 3> vel = pos - position;
	effect = (vel.Magnitude() - 0.02) / 0.04;
	if (effect < 0) effect = 0;
	else if (effect > 1) effect = 1;

	float bumpdiff = randgen.Get();
	float power = pow(bumpdiff, 32);
	if (power < 0) power = 0;
	else if (power > 0.2) power = 0.2;
	float veleffect = std::min(pow(vel.Magnitude() * ( 2.0 - stiffness), 3.0), 1.0);
	float bumpimpulse = power * 130.0 * veleffect;

	float k = 800.0 + stiffness * 800.0 * 4.0;
	float c = 2.0 * std::sqrt(k * mass) * 0.35;
	MATHVECTOR <float, 3> bumpforce = direction::Up * bumpimpulse;
	MATHVECTOR <float, 3> springforce = -displacement * k;
	MATHVECTOR <float, 3> damperforce = -velocity * c;
	
	velocity = velocity + (springforce + damperforce + bumpforce) * dt;
	displacement = displacement + velocity * dt;

	UpdatePosition(pos);
}

void CAMERA_MOUNT::UpdatePosition(const MATHVECTOR <float, 3> & newpos)
{
	MATHVECTOR <float, 3> dp = displacement;
	rotation.RotateVector(dp);
	MATHVECTOR <float, 3> maxdp(0.05, 0.05, 0.05);
	maxdp = maxdp * 1.0 / (stiffness + 1.0);
	for (int i = 0; i < 3; i++)
	{
		if (dp[i] > maxdp[i]) dp[i] = maxdp[i];
		if (dp[i] < -maxdp[i]) dp[i] = -maxdp[i];
	}
	dp[1] *= 0.5;

	position = newpos + (dp * effect) * offset_effect_strength;
}
