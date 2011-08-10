#ifndef _PERFORMANCE_TESTING_H
#define _PERFORMANCE_TESTING_H

#include "car.h"

#include <string>
#include <ostream>

class DynamicsWorld;

class PERFORMANCE_TESTING
{
public:
	PERFORMANCE_TESTING(DynamicsWorld & world);

	void Test(
		const std::string & carpath,
		const std::string & carname,
		const std::string & partspath,
		std::ostream & info_output,
		std::ostream & error_output);

private:
	DynamicsWorld & world;
	TRACKSURFACE surface;
	CAR car;
	std::string carstate;

	void SimulateFlatRoad();

	void ResetCar();

	void TestMaxSpeed(std::ostream & info_output, std::ostream & error_output);

	void TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output);
};

#endif
