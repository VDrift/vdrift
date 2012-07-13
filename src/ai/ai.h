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

#include "car.h"
#include "ai_car.h"
#include <string>
#include <vector>
#include <map>

class AI_Factory;

/// Manages all AI cars.
class AI
{
private:
	std::vector <AI_Car*> AI_Cars;
	std::map <std::string, AI_Factory*> AI_Factories;
	std::vector <float> empty_input;

public:
	AI();
	~AI();

	void add_car(CAR * car, float difficulty, const std::string & type = default_ai_type);
	void remove_car(CAR * car);
	void clear_cars();
	void update(float dt, const std::list <CAR> & othercars);
	const std::vector <float>& GetInputs(CAR * car) const; ///< Returns an empty vector if the car isn't AI-controlled.

	void AddAIFactory(const std::string& type_name, AI_Factory* factory);
	std::vector<std::string> ListFactoryTypes();

	void Visualize();

	static const std::string default_ai_type;
};

#endif //_AI_H
