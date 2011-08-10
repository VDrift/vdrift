#ifndef _TRACKSURFACE_H
#define _TRACKSURFACE_H

#include <string>

class TRACKSURFACE
{
public:
	enum TYPE
	{
		NONE = 0,
		ASPHALT = 1,
		GRASS = 2,
		GRAVEL = 3,
		CONCRETE = 4,
		SAND = 5,
		COBBLES = 6,
		NumTypes
	};

	void setType(const std::string & value)
	{
		if (value == "asphalt")			type = ASPHALT;
		else if (value == "grass")		type = GRASS;
		else if (value == "gravel")		type = GRAVEL;
		else if (value == "concrete") 	type = CONCRETE;
		else if (value == "sand")		type = SAND;
		else if (value == "cobbles")	type = COBBLES;
		else							type = NONE;
	}

	static const TRACKSURFACE * None()
	{
		static const TRACKSURFACE s;
		return &s;
	}

	TRACKSURFACE() :
		type(NONE),
		bumpWaveLength(1),
		bumpAmplitude(0),
		frictionNonTread(0),
		frictionTread(0),
		rollResistanceCoefficient(0),
		rollingDrag(0)
	{

	}

	TYPE type;
	float bumpWaveLength;
	float bumpAmplitude;
	float frictionNonTread;
	float frictionTread;
	float rollResistanceCoefficient;
	float rollingDrag;
};

#endif //_TRACKSURFACE_H

