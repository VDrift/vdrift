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

#ifndef _TRACKSURFACE_H
#define _TRACKSURFACE_H

#include <string>

class TrackSurface
{
public:
	enum Type
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

	static const TrackSurface * None()
	{
		static const TrackSurface s;
		return &s;
	}

	TrackSurface() :
		type(NONE),
		bumpWaveLength(1),
		bumpAmplitude(0),
		frictionNonTread(0),
		frictionTread(0),
		rollResistanceCoefficient(0),
		rollingDrag(0)
	{

	}

	Type type;
	float bumpWaveLength;
	float bumpAmplitude;
	float frictionNonTread;
	float frictionTread;
	float rollResistanceCoefficient;
	float rollingDrag;
};

#endif //_TRACKSURFACE_H

