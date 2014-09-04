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

#include <vector>
#include <iostream>
#include <sstream>

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
	std::tr1::shared_ptr<PTree> cfg;
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

	info_output << "Car dynamics loaded" << std::endl;
	info_output << carname << " Summary:\n" <<
			"Mass (kg) including driver and fuel: " << 1 / car.GetInvMass() << "\n" <<
			"Center of mass (m): " << car.GetCenterOfMass() << std::endl;

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
}

void PerformanceTesting::TestMaxSpeed(std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing maximum speed" << std::endl;

	double maxtime = 300.0;
	double t = 0.;
	double dt = 1/90.0;
	int i = 0;

	std::pair <float, float> maxspeed;
	maxspeed.first = 0;
	maxspeed.second = 0;
	float lastsecondspeed = 0;
	float stopthreshold = 0.001; //if the accel (in m/s^2) is less than this value, discontinue the testing

	float timeto60start = 0; //don't start the 0-60 clock until the car is moving at a threshold speed to account for the crappy launch that the autoclutch gives
	float timeto60startthreshold = 2.23; //threshold speed to start 0-60 clock in m/s
	//float timeto60startthreshold = 0.01; //threshold speed to start 0-60 clock in m/s
	float timeto60 = maxtime;

	float timetoquarter = maxtime;
	float quarterspeed = 0;

	std::string downforcestr = "N/A";

	ResetCar();

	while (t < maxtime)
	{
		if (car.GetTransmission().GetGear() == 1 &&
			car.GetEngine().GetRPM() > 0.8 * car.GetEngine().GetRedline())
		{
			carinput[CarInput::BRAKE] = 0.0f;
		}

		car.Update(carinput);

		world.update(dt);

		float car_speed = car.GetSpeed();

		if (car_speed > maxspeed.second)
		{
			maxspeed.first = t;
			maxspeed.second = car.GetSpeed();
			std::ostringstream dfs;
			dfs << -car.GetTotalAero()[2] << " N; " << -car.GetTotalAero()[2]/car.GetTotalAero()[0] << ":1 lift/drag";
			downforcestr = dfs.str();
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
			if (1)
			{
				if (car_speed - lastsecondspeed < stopthreshold && car_speed > 26.0)
				{
					//info_output << "Maximum speed attained at " << maxspeed.first << " s" << std::endl;
					break;
				}
				if (!car.GetEngine().GetCombustion())
				{
					error_output << "Car stalled during launch, t=" << t << std::endl;
					break;
				}
			}
			lastsecondspeed = car_speed;
			//std::cout << t << ", " << car_speed << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
		}

		t += dt;
		i++;
	}

	info_output << "Maximum speed: " << ConvertToMPH(maxspeed.second) << " MPH at " << maxspeed.first << " s" << std::endl;
	info_output << "Downforce at maximum speed: " << downforcestr << std::endl;
	info_output << "0-60 MPH time: " << timeto60-timeto60start << " s" << std::endl;
	info_output << "1/4 mile time: " << timetoquarter << " s" << " at " << ConvertToMPH(quarterspeed) << " MPH" << std::endl;
}

void PerformanceTesting::TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing stopping distance" << std::endl;

	double maxtime = 300.0;
	double t = 0.;
	double dt = 1/90.0;
	int i = 0;

	float stopthreshold = 0.1; //if the speed (in m/s) is less than this value, discontinue the testing
	btVector3 stopstart; //where the stopping starts
	float brakestartspeed = 26.82; //speed at which to start braking, in m/s (26.82 m/s is 60 mph)

	bool accelerating = true; //switches to false once 60 mph is reached

	ResetCar();

	car.SetABS(abs);

	while (t < maxtime)
	{
		if (accelerating)
		{
			carinput[CarInput::THROTTLE] = 1.0f;
			carinput[CarInput::BRAKE] = 0.0f;
		}
		else
		{
			carinput[CarInput::THROTTLE] = 0.0f;
			carinput[CarInput::BRAKE] = 1.0f;
		}

		car.Update(carinput);

		world.update(dt);

		float car_speed = car.GetSpeed();

		if (car_speed >= brakestartspeed && accelerating) //stop accelerating and hit the brakes
		{
			accelerating = false;
			stopstart = car.GetWheelPosition(WheelPosition(0));
			//std::cout << "hitting the brakes at " << t << ", " << car_speed << std::endl;
		}

		if (!accelerating && car_speed < stopthreshold)
		{
			break;
		}

		if (!car.GetEngine().GetCombustion())
		{
			error_output << "Car stalled during launch, t=" << t << std::endl;
			break;
		}

		if (i % (int)(1.0/dt) == 0) //every second
		{
			//std::cout << t << ", " << car.dynamics.GetSpeed() << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
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
	info_output << ": " << ConvertToFeet((stopend-stopstart).length()) << " ft" << std::endl;
}
