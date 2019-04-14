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

#ifndef _CARDIFFERENTIAL_H
#define _CARDIFFERENTIAL_H

#include "LinearMath/btScalar.h"

/// A differential that supports speed-sensitive limited slip functionality.
/// Epicyclic (torque splitting) operation is also provided.

struct CarDifferential
{
	btScalar final_drive = 4.1; ///< The gear ratio of the differential.
	btScalar anti_slip = 600; ///< This allows modelling of speed-sensitive limited-slip differentials. This is the maximum anti_slip torque that will be applied and, for speed-sensitive limited-slip differentials, the anti-slip multiplier that's always applied.
	btScalar anti_slip_torque = 0; ///< This allows modelling of torque sensitive limited-slip differentials. This is the anti_slip dependence on torque.
	btScalar anti_slip_torque_deceleration_factor = 0; ///< This allows modelling of torque sensitive limited-slip differentials that are 1.5 or 2-way. Set it to 0.0 for 1-way LSD, 1.0 for 2-way LSD, and somewhere in between for 1.5-way LSD.
	btScalar torque_split = 0.5; ///< This allows modelling of epicyclic differentials. This value ranges from 0.0 to 1.0 where 0.0 applies all torque to side1.
};

#endif
