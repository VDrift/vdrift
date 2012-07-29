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

class CAMERA_CHASE : public CAMERA
{
public:
	CAMERA_CHASE(const std::string & name);
	
	void SetOffset(const MATHVECTOR <float, 3> & value);

	void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing);

	void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, float dt);

private:
	MATHVECTOR <float, 3> focus;
	MATHVECTOR <float, 3> offset;
	bool posblend_on;
};

#endif // _CAMERA_CHASE_H
