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

#ifndef _CARINFO_H
#define _CARINFO_H

#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"

#include <string>

struct CarInfo
{
	std::string config;
	std::string driver;
	std::string name;
	std::string paint;
	std::string tire;
	std::string wheel;
	Vec3 hsv;
	float ailevel;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s, config);
		_SERIALIZE_(s, driver);
		_SERIALIZE_(s, name);
		_SERIALIZE_(s, paint);
		_SERIALIZE_(s, tire);
		_SERIALIZE_(s, wheel);
		_SERIALIZE_(s, hsv);
		_SERIALIZE_(s, ailevel);
		return true;
	}
};

#endif // _CARINFO_H
