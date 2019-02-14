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

#include "camera_mount.h"
#include "coordinatesystem.h"

CameraMount::CameraMount(const std::string & name) :
	Camera(name),
	stiffness(0),
	mass(1),
	offset_effect_strength(1),
	effect(0)
{
	rotation.LoadIdentity();
}

void CameraMount::Reset(const Vec3 & newpos, const Quat & newquat)
{
	Vec3 pos = offset;
	newquat.RotateVector(pos);
	rotation = newquat * offsetrot;
	displacement.Set(0, 0, 0);
	velocity.Set(0, 0, 0);
	UpdatePosition(pos + newpos);
}

void CameraMount::Update(const Vec3 & newpos, const Quat & newdir, float dt)
{
	rotation = newdir * offsetrot;

	Vec3 pos = offset;
	newdir.RotateVector(pos);
	pos = pos + newpos;

	Vec3 vel = pos - position;
	effect = (vel.Magnitude() - 0.02f) * 25;
	effect = Clamp(effect, 0.0f, 1.0f);

	float bumpdiff = randgen.Get();
	float power = std::pow(bumpdiff, 32.0f);
	power = Clamp(power, 0.0f, 0.2f);
	float veleffect = Min(std::pow(vel.Magnitude() * (2 - stiffness), 3.0f), 1.0f);
	float bumpimpulse = power * 130 * veleffect;

	float k = 800 + stiffness * 800 * 4;
	float c = 2 * std::sqrt(k * mass) * 0.35f;
	Vec3 bumpforce = Direction::Up * bumpimpulse;
	Vec3 springforce = -displacement * k;
	Vec3 damperforce = -velocity * c;

	velocity = velocity + (springforce + damperforce + bumpforce) * dt;
	displacement = displacement + velocity * dt;

	UpdatePosition(pos);
}

void CameraMount::UpdatePosition(const Vec3 & newpos)
{
	Vec3 dp = displacement;
	rotation.RotateVector(dp);
	Vec3 maxdp(0.05, 0.05, 0.05);
	maxdp = maxdp * 1 / (stiffness + 1);
	for (int i = 0; i < 3; i++)
	{
		dp[i] = Clamp(dp[i], -maxdp[i], maxdp[i]);
	}
	dp[1] *= 0.5f;

	position = newpos + (dp * effect) * offset_effect_strength;
}
