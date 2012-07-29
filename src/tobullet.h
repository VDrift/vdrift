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

#ifndef _TOBULLET_H
#define _TOBULLET_H

#include "mathvector.h"
#include "quaternion.h"
#include "matrix3.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btMatrix3x3.h"

inline btVector3 ToBulletVector(const MATHVECTOR <float, 3> & v)
{
	return btVector3(v[0], v[1], v[2]);
}

inline btQuaternion ToBulletQuaternion(const QUATERNION <float> & q)
{
	return btQuaternion(q.x(), q.y(), q.z(), q.w());
}

inline btQuaternion ToBulletQuaternion(const QUATERNION <double> & q)
{
	return btQuaternion(q.x(), q.y(), q.z(), q.w());
}

inline btMatrix3x3 ToBulletMatrix(const MATRIX3<float> & m)
{
	return btMatrix3x3(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
}

template <typename T> MATHVECTOR <T, 3> ToMathVector(const btVector3 & v)
{
	return MATHVECTOR <T, 3> (v.x(), v.y(), v.z());
}

template <typename T> QUATERNION <T> ToMathQuaternion(const btQuaternion & q)
{
	return QUATERNION <T> (q.x(), q.y(), q.z(), q.w());
}

#endif // _TOBULLET_H
