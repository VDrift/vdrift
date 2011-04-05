#ifndef _PERFORMANCE_TESTING_H
#define _PERFORMANCE_TESTING_H

#include "collision_world.h"
#include "car.h"

#include <string>
#include <ostream>

class PERFORMANCE_TESTING
{
public:
	PERFORMANCE_TESTING();
	void Test(
		const std::string & carpath,
		const std::string & carname,
		const std::string & partspath,
		std::ostream & info_output,
		std::ostream & error_output);
private:
	COLLISION_WORLD world;
	TRACKSURFACE surface;
	CAR car;
	std::string carstate;
	
	void SimulateFlatRoad();
	
	void ResetCar();
	
	void TestMaxSpeed(std::ostream & info_output, std::ostream & error_output);
	
	void TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output);
};

#endif
