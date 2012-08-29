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
#include "loadvehicle.h"
#include "physics/vehicleinfo.h"
#include "physics/world.h"
#include "content/contentmanager.h"
#include "cfg/ptree.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"

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

PERFORMANCE_TESTING::PERFORMANCE_TESTING(sim::World & world) :
	world(world), track(0), plane(0)
{
	surface.bumpWaveLength = 1;
	surface.bumpAmplitude = 0;
	surface.frictionNonTread = 1;
	surface.frictionTread = 1;
	surface.rollResistanceCoefficient = 1;
	surface.rollingDrag = 0;
}

PERFORMANCE_TESTING::~PERFORMANCE_TESTING()
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

void PERFORMANCE_TESTING::Test(
	const std::string & cardir,
	const std::string & carname,
	ContentManager & content,
	std::ostream & info_output,
	std::ostream & error_output)
{
	info_output << "Beginning car performance test with " << carname << std::endl;

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
	world.addCollisionObject(track);

	// load car config
	std::tr1::shared_ptr<PTree> cfg;
	content.load(cfg, cardir, carname + ".car");
	if (!cfg->size())
	{
		error_output << "Failed to load " << carname << ".car" << std::endl;
		return;
	}

	// load vehicle info
	sim::VehicleInfo vinfo;
	bool damage = false;
	btVector3 center(0.0, 0.0, 0.0);
	btVector3 size(1.0, 2.0, 0.25);
	if (!LoadVehicle(*cfg, damage, center, size, vinfo, error_output))
	{
		return;
	}

	// init vehicle
	// position is the center of a 2 x 4 x 1 meter box on track surface
	btVector3 position(0.0, -2.0, 0.5);
	btQuaternion rotation = btQuaternion::getIdentity();
	car.init(vinfo, position, rotation, world);
	car.setAutoShift(true);
	car.setAutoClutch(true);
	car.setABS(true);
	car.setTCS(true);
	car.startEngine();
	car.setBrake(1.0);

	// get initial state
	car.getState(carstate);

	info_output << carname << " Summary:\n";
	info_output << "Mass (kg) including driver and fuel: " << 1 / car.getInvMass() << "\n";
	//info_output << "Center of mass (m): " << car.getCenterOfMass() << std::endl;

	TestMaxSpeed(info_output, error_output);
	TestStoppingDistance(false, info_output, error_output);
	TestStoppingDistance(true, info_output, error_output);

	info_output << "Car performance test complete." << std::endl;
}

void PERFORMANCE_TESTING::ResetCar()
{
	car.setState(carstate);
}

void PERFORMANCE_TESTING::TestMaxSpeed(std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing maximum speed" << std::endl;

	ResetCar();

	double maxtime = 300.0;
	double t = 0.0;
	double dt = 0.05;
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
	while (t < maxtime)
	{
		// rev up before start
		if (car.getTransmission().getGear() == 0 &&
			car.getEngine().getRPM() > 0.7 * car.getEngine().getRPMLimit())
		{
			car.setGear(1);
		}
		car.setThrottle(1.0f);
		car.setBrake(0.0f);

		world.update(dt);

		float car_speed = car.getSpeed();

		if (car_speed > maxspeed.second)
		{
			maxspeed.first = t;
			maxspeed.second = car_speed;
			btVector3 aero = car.getTotalAero();

			std::stringstream dfs;
			dfs << -aero.z() << " N; lift/drag: " << aero.z() / aero.y();
			downforcestr = dfs.str();
		}

		if (car_speed < timeto60startthreshold)
			timeto60start = t;

		if (car_speed < 26.8224)
			timeto60 = t;

		if (car.getPosition().length() > 402.3 && timetoquarter == maxtime)
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
				if (!car.getEngine().getCombustion())
				{
					error_output << "Car stalled during launch, t=" << t << std::endl;
					break;
				}
			}
			lastsecondspeed = car_speed;
			//std::cout << t << ", " << car_speed << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
			//car.print(info_output, true, true, true, true);
		}

		t += dt;
		i++;
	}

	info_output << "Maximum speed: " << ConvertToMPH(maxspeed.second) << " MPH at " << maxspeed.first << " s" << std::endl;
	info_output << "Downforce at maximum speed: " << downforcestr << std::endl;
	info_output << "0-60 MPH time: " << timeto60-timeto60start << " s" << std::endl;
	info_output << "1/4 mile time: " << timetoquarter << " s" << " at " << ConvertToMPH(quarterspeed) << " MPH" << std::endl;
}

void PERFORMANCE_TESTING::TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing stopping distance" << std::endl;

	ResetCar();
	car.setABS(abs);
	car.setGear(1);

	double maxtime = 300.0;
	double t = 0.;
	double dt = 0.05;
	int i = 0;

	float stopthreshold = 0.1; //if the speed (in m/s) is less than this value, discontinue the testing
	btVector3 stopstart; //where the stopping starts
	float brakestartspeed = 26.82; //speed at which to start braking, in m/s (26.82 m/s is 60 mph)

	bool accelerating = true; //switches to false once 60 mph is reached
	while (t < maxtime)
	{
		if (accelerating)
		{
			car.setThrottle(1);
			car.setBrake(0);
		}
		else
		{
			car.setThrottle(0);
			car.setBrake(1);
			car.setGear(0);
		}

		world.update(dt);

		float car_speed = car.getSpeed();

		if (car_speed >= brakestartspeed && accelerating)
		{
			// stop accelerating and hit the brakes
			accelerating = false;
			stopstart = car.getPosition();

			// stopping distance estimation
			float d = car.getBrakingDistance(0);
			info_output << "60-0 stopping distance estimation: ";
			info_output << ConvertToFeet(d) << " ft" << std::endl;
		}

		if (!accelerating && car_speed < stopthreshold)
		{
			break;
		}

		if (!car.getEngine().getCombustion())
		{
			error_output << "Car stalled during launch, t=" << t << std::endl;
			//break;
		}

		if (i % (int)(1.0/dt) == 0) //every second
		{
			//std::cout << t << ", " << car.dynamics.GetSpeed() << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
		}

		t += dt;
		i++;
	}

	btVector3 stopend = car.getPosition();

	info_output << "60-0 stopping distance ";
	if (abs)
		info_output << "(ABS): ";
	else
		info_output << "(no ABS): ";
	info_output << ConvertToFeet((stopend - stopstart).length()) << " ft" << std::endl;
}
