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
