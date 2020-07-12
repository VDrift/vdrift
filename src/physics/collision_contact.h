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

#ifndef _COLLISION_CONTACT_H
#define _COLLISION_CONTACT_H

#include "tracksurface.h"
#include "coordinatesystem.h"
#include "LinearMath/btVector3.h"

class RoadPatch;
class btCollisionObject;

struct CollisionContact
{
	const btCollisionObject * col = 0;
	const TrackSurface * surface = TrackSurface::None();
	const RoadPatch * patch = 0;
	btVector3 position = btVector3(0,0,0);
	btVector3 normal = Direction::up;
	btScalar depth = 1;
	int patchid = -1;
};

#endif // _COLLISION_CONTACT_H
