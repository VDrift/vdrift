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

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "quaternion.h"
#include "mathvector.h"

class TRANSFORM
{
public:
	typedef QUATERNION<float> QUAT;
	typedef MATHVECTOR<float,3> VEC3;

	const QUAT & GetRotation() const {return rotation;}
	const VEC3 & GetTranslation() const {return translation;}
	void SetRotation(const QUAT & rot) {rotation = rot;}
	void SetTranslation(const VEC3 & trans) {translation = trans;}
	bool IsIdentityTransform() const {return (rotation == QUAT() && translation == VEC3());}
	void Clear() {rotation.LoadIdentity();translation.Set(0.0f);}

private:
	QUAT rotation;
	VEC3 translation;
};

#endif // _TRANSFORM_H
