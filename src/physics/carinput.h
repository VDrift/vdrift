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

#ifndef _CARINPUT_H
#define _CARINPUT_H

namespace CarInput
{
enum Enum
{
	THROTTLE = 0,
	NOS,
	BRAKE,
	HANDBRAKE,
	CLUTCH,
	STEER_LEFT,
	STEER_RIGHT,
	SHIFT_UP,
	SHIFT_DOWN,
	START_ENGINE,
	ABS_TOGGLE,
	TCS_TOGGLE,
	NEUTRAL,
	FIRST_GEAR,
	SECOND_GEAR,
	THIRD_GEAR,
	FOURTH_GEAR,
	FIFTH_GEAR,
	SIXTH_GEAR,
	REVERSE,
	ROLLOVER,
	INVALID
};
}

#endif

