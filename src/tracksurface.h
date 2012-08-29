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

#include "physics/surface.h"

class TRACKSURFACE : public sim::Surface
{
public:
	float pitch_variation;
	float max_gain;
	int sound_id; ///< hack, available sounds: asphalt = 0, gravel = 1, grass = 2

	TRACKSURFACE() :
		pitch_variation(0),
		max_gain(0),
		sound_id(0)
	{
		// ctor
	}

	static const TRACKSURFACE * None()
	{
		static const TRACKSURFACE s;
		return &s;
	}
};

#endif //_TRACKSURFACE_H

