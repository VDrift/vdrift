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

#ifndef _ENGINESOUNDINFO_H
#define _ENGINESOUNDINFO_H

struct ENGINESOUNDINFO
{
	float minrpm, maxrpm, naturalrpm, fullgainrpmstart, fullgainrpmend;
	enum { POWERON, POWEROFF, BOTH } power;
	size_t sound_source;

	ENGINESOUNDINFO() :
		minrpm(1.0),
		maxrpm(100000.0),
		naturalrpm(7000.0),
		fullgainrpmstart(minrpm),
		fullgainrpmend(maxrpm),
		power(BOTH),
		sound_source(0)
	{
		// ctor
	}

	bool operator < (const ENGINESOUNDINFO & other) const
	{
		return minrpm < other.minrpm;
	}
};

#endif // _ENGINESOUNDINFO_H
