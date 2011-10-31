#ifndef _CARFUELTANK_H
#define _CARFUELTANK_H

#include "LinearMath/btVector3.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>

class CARFUELTANK
{
friend class joeserialize::Serializer;
public:
	//default constructor makes an S2000-like car
	CARFUELTANK() :
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
