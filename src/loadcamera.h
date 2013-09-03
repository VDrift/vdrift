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

#ifndef _LOADCAMERA_H
#define _LOADCAMERA_H

#include <iosfwd>

class PTree;
class Camera;

// Read camera from config file
//
// [cam]
// name = foo			#required
// type = mount			#required types: mount, chase, orbit, free
// position = 0, 0, 0	#optional elative camera position
// lookat = 0, 1, 0		#optional used by mount only atm, determines view direction
// stiffness = 0		#optional used by mount
// fov = 90				#optional [40, 160], overrides global fov
//
Camera * LoadCamera(
	const PTree & cfg,
	float camera_bounce,
	std::ostream & error_output);

#endif  // _LOADCAMERA_H

