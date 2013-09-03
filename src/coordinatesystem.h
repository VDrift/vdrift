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

#ifndef _COORDINATESYSTEM_H
#define _COORDINATESYSTEM_H

#include "LinearMath/btVector3.h"
#include "mathvector.h"

namespace Direction
{

static const btVector3 right(1, 0, 0);
static const btVector3 forward(0, 1, 0);
static const btVector3 up(0, 0, 1);

static const Vec3 Right(1, 0, 0);
static const Vec3 Forward(0, 1, 0);
static const Vec3 Up(0, 0, 1);

}

#endif //_COORDINATESYSTEM_H
