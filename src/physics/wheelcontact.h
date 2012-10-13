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

#ifndef _SIM_WHEELCONTACT_H
#define _SIM_WHEELCONTACT_H

#include "constraintrow.h"

namespace sim
{

struct WheelContact
{
	ConstraintRow response;		///< contact normal constraint
	ConstraintRow friction1;	///< contact friction constraint
	ConstraintRow friction2;	///< contact friction constraint
	btRigidBody * bodyA;		///< contact body a
	btRigidBody * bodyB;		///< contact body b
	btVector3 rA;				///< contact position relative to bodyA
	btVector3 rB;				///< contact position relative to bodyB
	btScalar v1;				///< velocity along longitudinal constraint
	btScalar v2;				///< velocity along lateral constraint
	btScalar mus; 				///< surface friction coefficient
	btScalar camber;			///< wheel camber in degrees
	btScalar vR;				///< wheel rim velocity w * r
	int id;						///< custom id
};

}

#endif
