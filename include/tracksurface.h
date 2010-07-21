#ifndef _TRACKSURFACE_H
#define _TRACKSURFACE_H

#include <string>
#include <map>

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
		static const std::pair<std::string, TYPE> type_name[] =
		{
			std::pair<std::string, TYPE>("none", NONE),
			std::pair<std::string, TYPE>("asphalt", ASPHALT),
			std::pair<std::string, TYPE>("grass", GRASS),
			std::pair<std::string, TYPE>("gravel", GRAVEL),
			std::pair<std::string, TYPE>("concrete", CONCRETE),
			std::pair<std::string, TYPE>("sand", SAND),
			std::pair<std::string, TYPE>("cobbles", COBBLES)
		};
		static const std::map<std::string, TYPE> type_map(
			type_name,
			type_name + sizeof(type_name) / sizeof(type_name[0]));
		
		std::map<std::string, TYPE>::const_iterator i = type_map.find(value);
		if (i != type_map.end())
		{
			type = i->second;
		}
		else
		{
			type = NONE;
		}
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

