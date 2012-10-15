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

#include "ai.h"
#include <cassert>
// AI implementations:
#include "ai_car_standard.h"
#include "ai_car_experimental.h"

const std::string AI::default_ai_type = "standard";

AI::AI() : empty_input(CARINPUT::INVALID, 0.0)
{
	AddAIFactory("standard", new AI_Car_Standard_Factory());
	AddAIFactory("experimental", new AI_Car_Experimental_Factory());
}

AI::~AI()
{
	AI::clear_cars();

	// Free factories
	std::map <std::string, AI_Factory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++)
	{
		delete it->second;
	}

	AI_Factories.clear();
}

void AI::add_car(CAR * car, float difficulty, const std::string & type)
{
	assert(car);
	assert(AI_Factories.size() > 0);

	std::map <std::string, AI_Factory*>::iterator it = AI_Factories.find(type);
	assert(it != AI_Factories.end());
	AI_Factory* factory = it->second;
	AI_Car* aicar = factory->create(car, difficulty);
	AI_Cars.push_back(aicar);
}

void AI::remove_car(CAR * car)
{
	assert(car);

	for (size_t i = 0; i < AI_Cars.size(); i++)
	{
		if(AI_Cars[i]->GetCar() == car)
		{
			delete AI_Cars[i];
			AI_Cars.erase(AI_Cars.begin() + i);
			return;
		}
	}
}

void AI::clear_cars()
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		delete AI_Cars[i];
	}
	AI_Cars.clear();
}

void AI::update(float dt, const std::list <CAR> & othercars)
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		AI_Cars[i]->Update(dt, othercars);
	}
}

const std::vector <float> & AI::GetInputs(CAR * car) const
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		if (car == AI_Cars[i]->GetCar())
		{
			return AI_Cars[i]->GetInputs();
		}
	}

	return empty_input;
}

void AI::AddAIFactory(const std::string& type_name, AI_Factory* factory)
{
	AI_Factories.insert(std::make_pair(type_name, factory));
}

std::vector<std::string> AI::ListFactoryTypes()
{
	std::vector<std::string> ret;
	std::map<std::string, AI_Factory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++)
	{
		ret.push_back(it->first);
	}
	return ret;
}

void AI::Visualize()
{
#ifdef VISUALIZE_AI_DEBUG
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		AI_Cars[i]->Visualize();
	}
#endif
}

