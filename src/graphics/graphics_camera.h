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

#include "matrix4.h"

struct GraphicsCamera
{
	float fov;
	float view_distance;
	Vec3 pos;
	Quat rot;
	float w;
	float h;

	bool orthomode;
	Vec3 orthomin;
	Vec3 orthomax;

	GraphicsCamera() :
		fov(45),
		view_distance(10000),
		w(1),
		h(1),
		orthomode(false)
		{}
};

inline Mat4 GetViewMatrix(const GraphicsCamera & c)
{
	Mat4 m;
	c.rot.GetMatrix4(m);
	float t[4] = {-c.pos[0], -c.pos[1], -c.pos[2], 0};
	m.MultiplyVector4(t);
	m.Translate(t[0], t[1], t[2]);
	return m;
}

inline Mat4 GetProjMatrix(const GraphicsCamera & c)
{
	Mat4 m;
	if (c.orthomode)
		m.SetOrthographic(c.orthomin[0], c.orthomax[0], c.orthomin[1], c.orthomax[1], c.orthomin[2], c.orthomax[2]);
	else
		m.Perspective(c.fov, c.w / c.h, 0.1f, c.view_distance);
	return m;
}

inline Mat4 GetProjMatrixInv(const GraphicsCamera & c)
{
	assert(!c.orthomode);
	Mat4 m;
	m.InvPerspective(c.fov, c.w / c.h, 0.1f, c.view_distance);
	return m;
}

#endif // _GRAPHICS_CAMERA_H
