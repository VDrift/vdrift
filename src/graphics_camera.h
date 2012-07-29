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

#ifndef _GRAPHICS_CAMERA_H
#define _GRAPHICS_CAMERA_H

#include "mathvector.h"
#include "quaternion.h"

struct GRAPHICS_CAMERA
{
	float fov;
	float view_distance;
	MATHVECTOR <float, 3> pos;
	QUATERNION <float> orient;
	float w;
	float h;

	bool orthomode;
	MATHVECTOR <float, 3> orthomin;
	MATHVECTOR <float, 3> orthomax;

	GRAPHICS_CAMERA() :
		fov(45),
		view_distance(10000),
		w(1),
		h(1),
		orthomode(false)
		{}
};

#endif // _GRAPHICS_CAMERA_H
