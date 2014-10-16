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

#ifndef _AI_H
#define _AI_H

#include "ai_car.h"
#include <string>
#include <vector>
#include <map>

class AiFactory;

/// Manages all Ai cars.
class Ai
{
public:
	Ai();

	~Ai();

	void AddCar(const CarDynamics * car, float difficulty, const std::string & type = default_type);

	void RemoveCar(const CarDynamics * car);

	void ClearCars();

	void Update(float dt, const CarDynamics cars[], const int cars_num);

	///< Returns an empty vector if the car isn't AI-controlled.
	const std::vector<float> & GetInputs(const CarDynamics * car) const;

	void AddFactory(const std::string & type_name, AiFactory * factory);

	std::vector<std::string> ListFactoryTypes();

	void Visualize();

	static const std::string default_type;

private:
	std::vector <AiCar*> ai_cars;
	std::map <std::string, AiFactory*> ai_factories;
	std::vector <float> empty_input;
};

#endif //_AI_H
