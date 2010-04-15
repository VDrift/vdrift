#ifndef _TRACKSURFACE_H
#define _TRACKSURFACE_H

#include <string>
#include <assert.h>

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

	TYPE type;
	float bumpWaveLength;
	float bumpAmplitude;
	float frictionNonTread;
	float frictionTread;
	float rollResistanceCoefficient;
	float rollingDrag;
	std::string name;
	
	TRACKSURFACE()
	: type(NONE),
	  bumpWaveLength(1),
	  bumpAmplitude(0),
	  frictionNonTread(0),
	  frictionTread(0),
	  rollResistanceCoefficient(0),
	  rollingDrag(0)
	{

	}
	
	void setType(unsigned int i)
	{
		if (i < NumTypes)
		{
			type = (TYPE)i;
		}
		else
		{
			type = NumTypes;
		}
	}

	bool operator==(const TRACKSURFACE& t) const
	{
		return (type == t.type)
			&& (bumpWaveLength == t.bumpWaveLength)
			&& (bumpAmplitude == t.bumpAmplitude)
			&& (frictionNonTread == t.frictionNonTread)
			&& (frictionTread == t.frictionTread)
			&& (rollResistanceCoefficient == t.rollResistanceCoefficient)
			&& (rollingDrag == t.rollingDrag);
	}
	
	static const TRACKSURFACE * None()
	{
		static const TRACKSURFACE s;
		return &s;
	}
};

#endif //_TRACKSURFACE_H

