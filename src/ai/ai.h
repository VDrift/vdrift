#ifndef _AI_H
#define _AI_H

//#include "carinput.h"
//#include "reseatable_reference.h"
//#include "scenenode.h"

#include "car.h"

#include <string>
#include <vector>
#include <map>

class AI_Car;
class AI_Factory;

/// Manages all AI cars.
class AI
{
private:
	std::vector <float> empty_input;
	std::vector <AI_Car*> AI_Cars;
	std::map <std::string, AI_Factory*> AI_Factories;
public:
	AI();
	~AI();

	void add_car(CAR * car, float difficulty, std::string type = default_ai_type );
	void remove_car(CAR * car);
	void clear_cars();
	void update(float dt, const std::list <CAR> & othercars);
	const std::vector <float>& GetInputs(CAR * car) const; ///< returns an empty vector if the car isn't AI-controlled

	void AddAIFactory(const std::string& type_name, AI_Factory* factory);
	std::vector<std::string> ListFactoryTypes();

	void Visualize();

	static const std::string default_ai_type;
};

#endif //_AI_H
