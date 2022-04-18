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

#ifndef _CARCLUTCH_H
#define _CARCLUTCH_H

#include "LinearMath/btScalar.h"
#include "macros.h"

// default constructor makes an S2000-like car
struct CarClutchInfo
{
	btScalar friction = 0.4; ///< clutch friction coefficient
	btScalar max_pressure = 250000; ///< clutch pressure in Pa
	btScalar radius = 0.09;	///< mean effective radius
	btScalar area = 0.04; ///< frictiom surface area
};

class CarClutch
{
public:
	void Init(const CarClutchInfo & info)
	{
		max_torque = (info.max_pressure * info.area) * (info.friction * info.radius);
	}

	void SetPosition(btScalar value)
	{
		position = value;
	}

	btScalar GetPosition() const
	{
		return position;
	}

	btScalar GetTorque() const
	{
		return max_torque * position;
	}

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		out << "---Clutch---" << "\n";
		out << "Position: " << position << "\n";
		out << "Torque: " << GetTorque() << "\n";
	}

	template <class Serializer>
	bool Serialize(Serializer & s)
	{
		_SERIALIZE_(s, position);
		return true;
	}

private:
	btScalar max_torque = 360; ///< S2000-like car
	btScalar position = 0;
};

#endif
