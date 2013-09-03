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

#ifndef _CARFUELTANK_H
#define _CARFUELTANK_H

#include "LinearMath/btVector3.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CarFuelTank
{
friend class joeserialize::Serializer;
public:
	//default constructor makes an S2000-like car
	CarFuelTank() :
		capacity(0.0492),
		density(730.0),
		mass(730.0*0.0492),
		volume(0.0492)
	{
		// ctor
	}

	void DebugPrint(std::ostream & out) const
	{
		out << "---Fuel Tank---" << "\n";
		out << "Current volume: " << volume << "\n";
		out << "Capacity: " << capacity << "\n";
		out << "Mass: " << mass << "\n";
	}

	void SetCapacity(const btScalar & value)
	{
		capacity = value;
	}

	void SetDensity(const btScalar & value)
	{
		density = value;
		mass = density * volume;
	}

	void SetPosition(const btVector3 & value)
	{
		position = value;
	}


	const btVector3 & GetPosition() const
	{
		return position;
	}

	btScalar GetMass() const
	{
		return mass;
	}

	void SetVolume(const btScalar & value)
	{
		volume = value;
		mass = density * volume;
	}

	void Fill()
	{
		volume = capacity;
	}

	bool Empty() const
	{
		return (volume <= 0.0);
	}

	btScalar FuelPercent() const
	{
		return volume / capacity;
	}

	// consumption in kg
	void Consume(btScalar value)
	{
		mass = mass - value;
		if (mass < 0) mass = 0;
		volume = mass / density;
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, mass);
		_SERIALIZE_(s, volume);
		return true;
	}

private:
	//constants (not actually declared as const because they can be changed after object creation)
	btScalar capacity;
	btScalar density;
	btVector3 position;

	//variables
	btScalar mass;
	btScalar volume;

	void UpdateMass()
	{

	}


};

#endif
