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
	Ai::clear_cars();

	// Free factories
	std::map <std::string, AiFactory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++)
	{
		delete it->second;
	}

	AI_Factories.clear();
}

void Ai::add_car(Car * car, float difficulty, const std::string & type)
{
	assert(car);
	assert(AI_Factories.size() > 0);

	std::map <std::string, AiFactory*>::iterator it = AI_Factories.find(type);
	assert(it != AI_Factories.end());
	AiFactory* factory = it->second;
	AiCar* aicar = factory->create(car, difficulty);
	AI_Cars.push_back(aicar);
}

void Ai::remove_car(Car * car)
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

void Ai::clear_cars()
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		delete AI_Cars[i];
	}
	AI_Cars.clear();
}

void Ai::update(float dt, const std::list <Car> & othercars)
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		AI_Cars[i]->Update(dt, othercars);
	}
}

const std::vector <float> & Ai::GetInputs(Car * car) const
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

void Ai::AddFactory(const std::string& type_name, AiFactory* factory)
{
	AI_Factories.insert(std::make_pair(type_name, factory));
}

std::vector<std::string> Ai::ListFactoryTypes()
{
	std::vector<std::string> ret;
	std::map<std::string, AiFactory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++)
	{
		ret.push_back(it->first);
	}
	return ret;
}

void Ai::Visualize()
{
#ifdef VISUALIZE_AI_DEBUG
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		AI_Cars[i]->Visualize();
	}
#endif
}

