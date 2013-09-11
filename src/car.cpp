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
	steer_value(0),
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
	std::string carmodel;
	if (!cfg.get("body.mesh", carmodel, error_output))
		return false;

	std::tr1::shared_ptr<Model> model;
	content.load(model, carpath, carmodel);

	btVector3 size = ToBulletVector(model->GetSize());
	btVector3 center = ToBulletVector(model->GetCenter());
	btVector3 position = ToBulletVector(initial_position);
	btQuaternion rotation = ToBulletQuaternion(initial_orientation);

	if (!dynamics.Load(
		error_output, content, world,
		cfg, carpath, cartire,
		size, center, position, rotation,
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

void Car::SetPosition(const Vec3 & new_position)
{
	dynamics.SetPosition(ToBulletVector(new_position));
	dynamics.AlignWithGround();
}

void Car::Update(const std::vector <float> & inputs)
{
	// ensure that our inputs vector contains exactly one item per input
	assert(inputs.size() >= CarInput::INVALID);

	// recover from a rollover
	if (inputs[CarInput::ROLLOVER])
		dynamics.RolloverRecover();

	// set brakes
	dynamics.SetBrake(inputs[CarInput::BRAKE]);
	dynamics.SetHandBrake(inputs[CarInput::HANDBRAKE]);

	// do steering
	steer_value = inputs[CarInput::STEER_RIGHT] - inputs[CarInput::STEER_LEFT];
	dynamics.SetSteering(steer_value);

    // start the engine if requested
	if (inputs[CarInput::START_ENGINE])
		dynamics.StartEngine();

	// do shifting
	int gear_change = 0;
	if (inputs[CarInput::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CarInput::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = dynamics.GetTransmission().GetGear();
	int new_gear = cur_gear + gear_change;

	if (inputs[CarInput::REVERSE])
		new_gear = -1;
	if (inputs[CarInput::NEUTRAL])
		new_gear = 0;
	if (inputs[CarInput::FIRST_GEAR])
		new_gear = 1;
	if (inputs[CarInput::SECOND_GEAR])
		new_gear = 2;
	if (inputs[CarInput::THIRD_GEAR])
		new_gear = 3;
	if (inputs[CarInput::FOURTH_GEAR])
		new_gear = 4;
	if (inputs[CarInput::FIFTH_GEAR])
		new_gear = 5;
	if (inputs[CarInput::SIXTH_GEAR])
		new_gear = 6;

	float throttle = inputs[CarInput::THROTTLE];
	float clutch = 1 - inputs[CarInput::CLUTCH];
	float nos = inputs[CarInput::NOS];

	nos_active = nos > 0;

	dynamics.ShiftGear(new_gear);
	dynamics.SetThrottle(throttle);
	dynamics.SetClutch(clutch);
	dynamics.SetNOS(nos);

	// do driver aid toggles
	if (inputs[CarInput::ABS_TOGGLE])
		dynamics.SetABS(!dynamics.GetABSEnabled());

	if (inputs[CarInput::TCS_TOGGLE])
		dynamics.SetTCS(!dynamics.GetTCSEnabled());

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
	_SERIALIZE_(s, steer_value);
	return true;
}
