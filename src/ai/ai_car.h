#ifndef _AI_CAR_H
#define _AI_CAR_H

#include "carinput.h"

#include <vector>
#include <list>

class CAR;

/// AI Car controller interface.
class AI_Car
{
protected:
	CAR* car;
	float difficulty;

	/// Contains the car inputs, which is the output of the AI.
	/// The vector is indexed by CARINPUT values.
	std::vector <float> inputs;

public:
	AI_Car(CAR* _car, float _difficulty) :
		car(_car), difficulty(_difficulty), inputs(CARINPUT::INVALID, 0.0)
	{ }
	virtual ~AI_Car(){}

	CAR*						GetCar() { return car; }
	float						GetDifficulty() { return difficulty; }
	const std::vector<float>&	GetInputs() { return inputs; }

	virtual void Update(float dt, const std::list<CAR>& othercars) = 0;

	/// This is optional for drawing debug stuff.
	/// It will only be called, when VISUALIZE_AI_DEBUG macro is defined.
	virtual void Visualize() { }
};

#endif // _AI_CAR_H