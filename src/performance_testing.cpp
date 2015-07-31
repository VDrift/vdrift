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

#include "performance_testing.h"
#include "physics/carinput.h"
#include "physics/dynamicsworld.h"
#include "physics/tracksurface.h"
#include "content/contentmanager.h"
#include "cfg/ptree.h"

#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <ctime>

static inline float ConvertToMPH(float ms)
{
	return ms * 2.23693629;
}

static inline float ConvertToFeet(float meters)
{
	return meters * 3.2808399;
}

PerformanceTesting::PerformanceTesting(DynamicsWorld & world) :
	world(world), track(0), plane(0)
{
	surface.type = TrackSurface::ASPHALT;
	surface.bumpWaveLength = 1;
	surface.bumpAmplitude = 0;
	surface.frictionNonTread = 1;
	surface.frictionTread = 1;
	surface.rollResistanceCoefficient = 1;
	surface.rollingDrag = 0;
}

PerformanceTesting::~PerformanceTesting()
{
	if (track)
	{
		world.removeCollisionObject(track);
		delete track;
	}
	if (plane)
	{
		delete plane;
	}
}

void PerformanceTesting::Test(
	const std::string & cardir,
	const std::string & carname,
	ContentManager & content,
	std::ostream & info_output,
	std::ostream & error_output)
{
	info_output << "Beginning car performance test on " << carname << std::endl;

	// init track
	assert(!track);
	assert(!plane);
	btVector3 planeNormal(0, 0, 1);
	btScalar planeConstant = 0;
	plane = new btStaticPlaneShape(planeNormal, planeConstant);
	plane->setUserPointer(static_cast<void*>(&surface));
	track = new btCollisionObject();
	track->setCollisionShape(plane);
	track->setActivationState(DISABLE_SIMULATION);
	track->setUserPointer(static_cast<void*>(&surface));
	world.addCollisionObject(track);

	//load the car dynamics
	std::shared_ptr<PTree> cfg;
	content.load(cfg, cardir, carname + ".car");
	if (!cfg->size())
	{
		return;
	}

	// position is the center of a 2 x 4 x 1 meter box on track surface
	btVector3 pos(0.0, -2.0, 0.5);
	btQuaternion rot = btQuaternion::getIdentity();
	const std::string tire = "";
	const bool damage = false;
	if (!car.Load(*cfg, cardir, tire, pos, rot, damage, world, content, error_output))
	{
		return;
	}

	btVector3 cm = -car.GetCenterOfMassOffset();
	info_output << carname << " Summary:\n"
		<< "Mass including driver and fuel: " << 1 / car.GetInvMass() << " kg\n"
		<< "Center of mass: " << cm[0] << ", " << cm[1] << ", " << cm[2] << " m\n"
		<< "Estimated top speed: " << ConvertToMPH(car.GetMaxSpeedMPS()) << " MPH" << std::endl;

	std::ostringstream statestream;
	joeserialize::BinaryOutputSerializer serialize_output(statestream);
	if (!car.Serialize(serialize_output))
	{
		error_output << "Serialization error" << std::endl;
	}
	//else info_output << "Car state: " << statestream.str();
	carstate = statestream.str();

	TestMaxSpeed(info_output, error_output);
	TestStoppingDistance(false, info_output, error_output);
	TestStoppingDistance(true, info_output, error_output);

	info_output << "Car performance test complete." << std::endl;
}

void PerformanceTesting::ResetCar()
{
	std::istringstream statestream(carstate);
	joeserialize::BinaryInputSerializer serialize_input(statestream);
	car.Serialize(serialize_input);

	car.SetAutoShift(true);
	car.SetAutoClutch(true);
	car.SetTCS(true);
	car.SetABS(true);

	carinput.clear();
	carinput.resize(CarInput::INVALID, 0.0f);
	carinput[CarInput::THROTTLE] = 1.0f;
	carinput[CarInput::BRAKE] = 1.0f;
	carinput[CarInput::CLUTCH] = 1.0f;
}

void PerformanceTesting::TestMaxSpeed(std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing top speed" << std::endl;

	float maxtime = 300.0;
	float t = 0.;
	float dt = 1/90.0;
	int i = 0;

	std::pair <float, float> maxspeed(0, 0);
	float maxlift = 0;
	float maxdrag = 0;
	float lastsecondspeed = 0;
	float stopthreshold = 0.001; //if the accel (in m/s^2) is less than this value, discontinue the testing

	float timeto60start = 0; //don't start the 0-60 clock until the car is moving at a threshold speed to account for the crappy launch that the autoclutch gives
	float timeto60startthreshold = 1; //threshold speed to start 0-60 clock in m/s
	float timeto60 = maxtime;

	float timetoquarter = maxtime;
	float quarterspeed = 0;

	ResetCar();

	clock_t cpu_timer_start = clock();
	while (t < maxtime)
	{
		if (car.GetTransmission().GetGear() == 1 &&
			car.GetEngine().GetRPM() > 0.8 * car.GetEngine().GetRedline())
		{
			carinput[CarInput::BRAKE] = 0.0f;
			carinput[CarInput::CLUTCH] = 0.0f;
		}

		car.Update(carinput);

		world.update(dt);

		float car_speed = car.GetSpeed();
		if (car_speed > maxspeed.second)
		{
			maxspeed.first = t;
			maxspeed.second = car_speed;
			maxlift = car.GetTotalAero()[2];
			maxdrag = car.GetTotalAero()[1];
		}

		if (car_speed < timeto60startthreshold)
			timeto60start = t;

		if (car_speed < 26.8224)
			timeto60 = t;

		if (car.GetCenterOfMass().length() > 402.3 && timetoquarter == maxtime)
		{
			//quarter mile!
			timetoquarter = t - timeto60start;
			quarterspeed = car_speed;
		}

		if (i % (int)(1.0/dt) == 0) //every second
		{
			if (car_speed - lastsecondspeed < stopthreshold && car_speed > 26.0)
			{
				//info_output << "Maximum speed attained at " << maxspeed.first << " s" << std::endl;
				break;
			}

			if (t > 0 && !car.GetEngine().GetCombustion())
			{
				error_output << "Car stalled during launch, t=" << t << std::endl;
			}

			lastsecondspeed = car_speed;
			//std::cout << t << ", " << car_speed << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
		}

		t += dt;
		i++;
	}
	clock_t cpu_timer_stop = clock();
	float sim_perf = t / float(cpu_timer_stop - cpu_timer_start) * CLOCKS_PER_SEC;

	info_output << "Top speed: " << ConvertToMPH(maxspeed.second) << " MPH at " << maxspeed.first << " s\n";
	info_output << "Downforce at top speed: " << -maxlift << " N\n";
	info_output << "Drag at top speed: " << -maxdrag << " N\n";
	info_output << "0-60 MPH time: " << timeto60 - timeto60start << " s\n";
	info_output << "1/4 mile time: " << timetoquarter << " s at " << ConvertToMPH(quarterspeed) << " MPH" << std::endl;
	info_output << "Simulation performance: " << sim_perf << std::endl;
}

void PerformanceTesting::TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing stopping distance" << std::endl;

	float maxtime = 300.0;
	float t = 0.;
	float dt = 1/90.0;
	int i = 0;

	float stopthreshold = 0.1; //if the speed (in m/s) is less than this value, discontinue the testing
	btVector3 stopstart; //where the stopping starts
	float brakestartspeed = 26.82; //speed at which to start braking, in m/s (26.82 m/s is 60 mph)

	// wheel lockup speeds during braking
	float front_lockup_speed = 0.0f;
	float rear_lockup_speed = 0.0f;

	bool accelerating = true; //switches to false once 60 mph is reached

	ResetCar();

	car.SetABS(abs);

	while (t < maxtime)
	{
		if (accelerating && car.GetTransmission().GetGear() == 1 &&
			car.GetEngine().GetRPM() > 0.8 * car.GetEngine().GetRedline())
		{
			carinput[CarInput::BRAKE] = 0.0f;
			carinput[CarInput::CLUTCH] = 0.0f;
		}

		car.Update(carinput);

		world.update(dt);

		float car_speed = car.GetSpeed();

		if (car_speed >= brakestartspeed && accelerating) //stop accelerating and hit the brakes
		{
			stopstart = car.GetWheelPosition(WheelPosition(0));

			carinput[CarInput::THROTTLE] = 0.0f;
			carinput[CarInput::BRAKE] = 1.0f;
			accelerating = false;

			//info_output << "hitting the brakes at " << t << ", " << car_speed << std::endl;
		}

		if (!accelerating && car_speed < stopthreshold)
		{
			break;
		}

		if (!accelerating)
		{
			if (!(front_lockup_speed > 0.0) && car.GetWheel(WheelPosition(0)).GetRPM() < 0.001f)
			{
				front_lockup_speed = car_speed;
			}
			if (!(rear_lockup_speed > 0.0) && car.GetWheel(WheelPosition(3)).GetRPM() < 0.001f)
			{
				rear_lockup_speed = car_speed;
			}
		}

		if (t > 0 && !car.GetEngine().GetCombustion())
		{
			error_output << "Car stalled during launch, t=" << t << std::endl;
		}

		if (i % (int)(1.0/dt) == 0) //every second
		{
			//info_output << t
			//	<< ", " << car_speed
			//	<< ", " << car.GetWheel(WheelPosition(0)).GetAngularVelocity()
			//	<< ", " << car.GetBrake(WheelPosition(0)).GetBrakeFactor()
			//	<< std::endl;
		}

		t += dt;
		i++;
	}

	btVector3 stopend = car.GetWheelPosition(WheelPosition(0));

	info_output << "60-0 stopping distance ";
	if (abs)
		info_output << "(ABS)";
	else
		info_output << "(no ABS)";
	info_output << ": " << ConvertToFeet((stopend-stopstart).length()) << " ft\n"
		<< "Wheel lockup speed " << ConvertToMPH(front_lockup_speed)
		<< ", " << ConvertToMPH(rear_lockup_speed) << std::endl;
}
