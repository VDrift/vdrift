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

#ifndef _SIM_VEHICLESTATE_H
#define _SIM_VEHICLESTATE_H

#include "LinearMath/btAlignedObjectArray.h"
#include "LinearMath/btTransform.h"

namespace sim
{

struct VehicleState
{
	btAlignedObjectArray<btScalar> shaft_angvel;
	btTransform transform;
	btVector3 lin_velocity;
	btVector3 ang_velocity;
	btScalar brake;
	btScalar clutch;
	btScalar shift_time;
	btScalar tacho_rpm;
	int gear;
	bool shifted;
	bool auto_shift;
	bool auto_clutch;
	bool abs_enabled;
	bool tcs_enabled;
};

}

#endif // _SIM_VEHICLESTATE_H
