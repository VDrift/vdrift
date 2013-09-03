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

class Transform
{
public:
	const Quat & GetRotation() const {return rotation;}
	const Vec3 & GetTranslation() const {return translation;}
	void SetRotation(const Quat & rot) {rotation = rot;}
	void SetTranslation(const Vec3 & trans) {translation = trans;}
	bool IsIdentityTransform() const {return (rotation == Quat() && translation == Vec3());}
	void Clear() {rotation.LoadIdentity();translation.Set(0.0f);}

private:
	Quat rotation;
	Vec3 translation;
};

#endif // _TRANSFORM_H
