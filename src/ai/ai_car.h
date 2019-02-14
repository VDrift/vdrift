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

#ifndef _AI_CAR_H
#define _AI_CAR_H

#include "physics/carinput.h"
#include <vector>

class CarDynamics;

/// AI Car controller interface.
class AiCar
{
public:
	AiCar(unsigned carid, float difficulty);

	virtual ~AiCar();

	unsigned GetCarId() const;

	const std::vector<float> & GetInputs() const;

	virtual void Update(float dt, const CarDynamics cars[], const unsigned cars_num) = 0;

	/// This is optional for drawing debug stuff.
	/// It will only be called, when VISUALIZE_AI_DEBUG macro is defined.
	virtual void Visualize();

protected:
	const unsigned carid;
	const float difficulty;

	/// Contains the car inputs, which is the output of the AI.
	/// The vector is indexed by CARINPUT values.
	std::vector <float> inputs;
};


inline AiCar::AiCar(unsigned carid, float difficulty) :
	carid(carid),
	difficulty(difficulty),
	inputs(CarInput::INVALID, 0.0)
{
	// ctor
}

inline AiCar::~AiCar()
{
	// dtor
}

inline unsigned AiCar::GetCarId() const
{
	return carid;
}

inline const std::vector<float> & AiCar::GetInputs() const
{
	return inputs;
}

inline void AiCar::Visualize()
{
	// optional
}

#endif // _AI_CAR_H
