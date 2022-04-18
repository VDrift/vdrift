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

#ifndef _CONECULL_H
#define _CONECULL_H

#include "mathvector.h"
#include "minmax.h"

struct Cone
{
	Vec3 pos;
	Vec3 dir;
	float sina;
	float rcosa2;
	
	Cone(Vec3 cpos, Vec3 cdir, float csina)
	{
		pos = cpos;
		dir = cdir;
		sina = csina;
		rcosa2 = 1 / (1 - csina * csina);
	}

	// cull sphere agains cone
	bool cull(Vec3 sphere_pos, float sphere_radius)
	{
		Vec3 r = sphere_pos - pos;
/*
		// cone plane (shifted by -radius sina) vs point
		float rd = r.dot(dir);
		if (rd > sphere_radius * sina)
		{
			// cone vs sphere center shifted by radius / sina
			float cd = rd * sina + sphere_radius;
			return r.dot(r) > cd * cd * rcosa2 + rd * rd;
		}
		// cone origin vs sphere
		return r.dot(r) > sphere_radius * sphere_radius;
*/
		// optimized
		float rd = r.dot(dir);
		float rr = r.dot(r);
		float cd = rd * sina + sphere_radius;
		float e = sphere_radius * sphere_radius;
		float f = cd * cd * rcosa2 + rd * rd;
		return rr > Max(e, f); // rr > e && rr > f
	}
};

#endif // _CONECULL_H
