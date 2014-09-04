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

#include "car.h"
#include "physics/carinput.h"
#include "graphics/model.h"
#include "content/contentmanager.h"
#include "cfg/ptree.h"

Car::Car() :
	nos_active(false),
	sector(-1)
{
	// ctor
}

Car::~Car()
{
	// dtor
}

bool Car::LoadGraphics(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & carname,
	const std::string & carwheel,
	const std::string & carpaint,
	const Vec3 & carcolor,
	const int anisotropy,
	const float camerabounce,
	ContentManager & content,
	std::ostream & error_output)
{
	cartype = carname;

	return graphics.Load(cfg, carpath, carname, carwheel, carpaint,
		carcolor, anisotropy, camerabounce, content, error_output);
}

bool Car::LoadPhysics(
	std::ostream & error_output,
	ContentManager & content,
	DynamicsWorld & world,
	const PTree & cfg,
	const std::string & carpath,
	const std::string & cartire,
	const Vec3 & initial_position,
	const Quat & initial_orientation,
	const bool defaultabs,
	const bool defaulttcs,
	const bool damage)
{
	btVector3 position = ToBulletVector(initial_position);
	btQuaternion rotation = ToBulletQuaternion(initial_orientation);

	if (!dynamics.Load(
		error_output, content, world,
		cfg, carpath, cartire,
		position, rotation,
		damage))
	{
		return false;
	}

	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);

	mz_nominalmax = 0.05f * 9.81f / dynamics.GetInvMass(); // fixme: make this a steering feedback parameter

	return true;
}

bool Car::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	Sound & soundsystem,
	ContentManager & content,
	std::ostream & error_output)
{
	return sound.Load(carpath, carname, soundsystem, content, error_output);
}

void Car::Update(const std::vector <float> & inputs)
{
	assert(inputs.size() >= CarInput::INVALID);

	nos_active = inputs[CarInput::NOS] > 0;

	dynamics.Update(inputs);

	graphics.Update(inputs);
}

void Car::Update(double dt)
{
	graphics.Update(dynamics);
	sound.Update(dynamics, dt);
}

void Car::SetInteriorView(bool value)
{
	graphics.EnableInteriorView(value);
	sound.EnableInteriorSounds(value);
}

bool Car::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, dynamics);
	return true;
}
