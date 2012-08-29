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

#ifndef _LOADVEHICLE_H
#define _LOADVEHICLE_H

#include <ostream>

class PTree;
class btVector3;
namespace sim { struct VehicleInfo; }

bool LoadVehicle(
	const PTree & cfg,
	const bool damage,
	const btVector3 & modelcenter,
	const btVector3 & modelsize,
	sim::VehicleInfo & info,
	std::ostream & error);

#endif // _LOADVEHICLE_H
