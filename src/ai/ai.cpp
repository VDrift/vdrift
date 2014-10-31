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

const std::string Ai::default_type = "aistd";

Ai::Ai() : empty_input(CarInput::INVALID, 0.0)
{
	AddFactory("aistd", new AiCarStandardFactory());
	AddFactory("aiexp", new AiCarExperimentalFactory());
}

Ai::~Ai()
{
	Ai::ClearCars();

	std::map <std::string, AiFactory*>::iterator it;
	for (it = ai_factories.begin(); it != ai_factories.end(); it++)
	{
		delete it->second;
	}
	ai_factories.clear();
}

void Ai::AddCar(const CarDynamics * car, float difficulty, const std::string & type)
{
	assert(car);
	assert(!ai_factories.empty());

	std::map <std::string, AiFactory*>::iterator it = ai_factories.find(type);
	assert(it != ai_factories.end());
	AiFactory * factory = it->second;
	AiCar * aicar = factory->Create(car, difficulty);
	ai_cars.push_back(aicar);
}

void Ai::RemoveCar(const CarDynamics * car)
{
	assert(car);

	for (size_t i = 0; i < ai_cars.size(); i++)
	{
		if(ai_cars[i]->GetCar() == car)
		{
			delete ai_cars[i];
			ai_cars.erase(ai_cars.begin() + i);
			return;
		}
	}
}

void Ai::ClearCars()
{
	int size = ai_cars.size();
	for (int i = 0; i < size; i++)
	{
		delete ai_cars[i];
	}
	ai_cars.clear();
}

void Ai::Update(float dt, const CarDynamics cars[], const int cars_num)
{
	int size = ai_cars.size();
	for (int i = 0; i < size; i++)
	{
		ai_cars[i]->Update(dt, cars, cars_num);
	}
}

const std::vector<float> & Ai::GetInputs(const CarDynamics * car) const
{
	int size = ai_cars.size();
	for (int i = 0; i < size; i++)
	{
		if (car == ai_cars[i]->GetCar())
		{
			return ai_cars[i]->GetInputs();
		}
	}
	return empty_input;
}

void Ai::AddFactory(const std::string & type_name, AiFactory * factory)
{
	ai_factories.insert(std::make_pair(type_name, factory));
}

std::vector<std::string> Ai::ListFactoryTypes()
{
	std::vector<std::string> ret;
	std::map<std::string, AiFactory*>::iterator it;
	for (it = ai_factories.begin(); it != ai_factories.end(); it++)
	{
		ret.push_back(it->first);
	}
	return ret;
}

void Ai::Visualize()
{
#ifdef VISUALIZE_AI_DEBUG
	int size = ai_cars.size();
	for (int i = 0; i < size; i++)
	{
		ai_cars[i]->Visualize();
	}
#endif
}

