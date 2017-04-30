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

Ai::Ai()
{
	AddFactory("aistd", new AiCarStandardFactory());
	AddFactory("aiexp", new AiCarExperimentalFactory());
}

Ai::~Ai()
{
	Ai::ClearCars();

	for (auto & factory : ai_factories)
	{
		delete factory.second;
	}
	ai_factories.clear();
}

unsigned Ai::AddCar(unsigned carid, float difficulty, const std::string & type)
{
	auto it = ai_factories.find(type);
	assert(it != ai_factories.end());
	AiFactory * factory = it->second;

	AiCar * aicar = factory->Create(carid, difficulty);
	ai_cars.push_back(aicar);

	return ai_cars.size() - 1;
}

void Ai::ClearCars()
{
	for (auto ai_car : ai_cars)
	{
		delete ai_car;
	}
	ai_cars.clear();
}

void Ai::Update(float dt, const CarDynamics cars[], const int cars_num)
{
	for (auto ai_car : ai_cars)
	{
		ai_car->Update(dt, cars, cars_num);
	}
}

const std::vector<float> & Ai::GetInputs(unsigned id) const
{
	return ai_cars[id]->GetInputs();
}

void Ai::AddFactory(const std::string & type_name, AiFactory * factory)
{
	ai_factories.insert(std::make_pair(type_name, factory));
}

std::vector<std::string> Ai::ListFactoryTypes()
{
	std::vector<std::string> ret;
	for (const auto & factory : ai_factories)
	{
		ret.push_back(factory.first);
	}
	return ret;
}

void Ai::Visualize()
{
#ifdef VISUALIZE_AI_DEBUG
	for (auto ai_car : ai_cars)
	{
		ai_car->Visualize();
	}
#endif
}

