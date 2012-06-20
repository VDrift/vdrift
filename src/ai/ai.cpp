#include "ai.h"
#include "car.h"
#include "ai_car.h"

// AI implementations:
#include "ai_car_standard.h"
#include "ai_car_experimental.h"

#include <cassert>

const std::string AI::default_ai_type = "Standard AI";

AI::AI() :
	empty_input(CARINPUT::INVALID, 0.0)
{
	AddAIFactory(default_ai_type, new AI_Car_Standard_Factory());
	AddAIFactory("Experimental AI", new AI_Car_Experimental_Factory());
}

AI::~AI(){
	AI::clear_cars();

	//free factories
	std::map <std::string, AI_Factory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++) {
		delete it->second;
	}
	AI_Factories.clear();
}

const std::vector <float> & AI::GetInputs(CAR * car) const
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++) {
		if (car == AI_Cars[i]->GetCar()) {
			return AI_Cars[i]->GetInputs();
		}
	}

	//Trying to get the input of a not AI controlled car.
	assert(1);

	return empty_input;
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

void AI::add_car(CAR * car, float difficulty, std::string type)
{
	assert(car);
	assert(AI_Factories.size()>0);

	std::map <std::string, AI_Factory*>::iterator it;
	it = AI_Factories.find(type);
	assert(it != AI_Factories.end());
	AI_Factory* factory = it->second;
	AI_Car* aicar = factory->create(car, difficulty);
	AI_Cars.push_back(aicar);
}

void AI::remove_car(CAR * car)
{
	assert(car);

	for (unsigned int i = 0; i < AI_Cars.size(); i++)
	{
		if(AI_Cars[i]->GetCar() == car)
		{
			delete AI_Cars[i];
			AI_Cars.erase(AI_Cars.begin() + i);
			return;
		}
	}

	//Car not found.
	assert(1);
}

void AI::update(float dt, const std::list <CAR> & othercars)
{
	int size = AI_Cars.size();
	for (int i = 0; i < size; i++)
	{
		AI_Cars[i]->Update(dt, othercars);
	}
}

void AI::AddAIFactory(const std::string& type_name, AI_Factory* factory){
	AI_Factories.insert(std::make_pair(type_name, factory));
}
std::vector<std::string> AI::ListFactoryTypes(){
	std::vector<std::string> ret;
	std::map<std::string, AI_Factory*>::iterator it;
	for (it = AI_Factories.begin(); it != AI_Factories.end(); it++) {
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

