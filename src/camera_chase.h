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

#ifndef _CAMERA_CHASE_H
#define _CAMERA_CHASE_H

#include "camera.h"

class CameraChase : public Camera
{
public:
	CameraChase(const std::string & name);

	void SetOffset(const Vec3 & value);

	void Reset(const Vec3 & newfocus, const Quat & focus_facing);

	void Update(const Vec3 & newfocus, const Quat & focus_facing, float dt);

private:
	Vec3 focus;
	Vec3 offset;
	bool posblend_on;
};

#endif // _CAMERA_CHASE_H
