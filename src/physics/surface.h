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

#ifndef _SIM_SURFACE_H
#define _SIM_SURFACE_H

namespace sim
{

struct Surface
{
	float bumpWaveLength;
	float bumpAmplitude;
	float frictionNonTread;
	float frictionTread;
	float rollResistanceCoefficient;
	float rollingDrag;

	Surface();

	static const Surface * None();
};

// implementation

inline Surface::Surface() :
	bumpWaveLength(1),
	bumpAmplitude(0),
	frictionNonTread(0),
	frictionTread(0),
	rollResistanceCoefficient(0),
	rollingDrag(0)
{
	// ctor
}

inline const Surface * Surface::None()
{
	static const Surface s;
	return &s;
}

}

#endif

