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

#ifndef _SIM_CONSTRAINTROW_H
#define _SIM_CONSTRAINTROW_H

#include "LinearMath/btVector3.h"

namespace sim
{

struct ConstraintRow
{
	btVector3 normal;
	btVector3 angularCompA;
	btVector3 angularCompB;
	btScalar rhs;
	btScalar cfm;
	btScalar jacDiagInv;
	btScalar lowerLimit;
	btScalar upperLimit;
	btScalar accumImpulse;
};

}

#endif
