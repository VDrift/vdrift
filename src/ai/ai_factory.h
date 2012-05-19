#ifndef _AI_FACTORY_H
#define _AI_FACTORY_H

class CAR;
class AI_Car;

/// Abstract Factory for the AI implementations
class AI_Factory
{
public:
	virtual AI_Car* create(CAR * car, float difficulty) = 0;
};

#endif // _AI_FACTORY_H