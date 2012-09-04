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

#ifndef _SIM_VEHICLEINPUT_H
#define _SIM_VEHICLEINPUT_H

namespace sim
{

struct VehicleInput
{
	enum Control {STEER, THROTTLE, BRAKE, HBRAKE, CLUTCH, NOS, CTRLNUM};
	enum Logic {STARTENG=1, RECOVER=2, TCS=4, ABS=8, AUTOCLUTCH=16, AUTOSHIFT=32};
	float controls[CTRLNUM];	///< controls are normalized [0, 1], steering [-1, 1]
	short shiftgear;			///< new gear offset relative to current
	short logic;				///< vehicle logic

	/// some convenience functions
	void set(Control n, btScalar value);
	void set(Logic n, bool value);
	void clear();
};

// implementation

inline void VehicleInput::set(Control n, btScalar value)
{
	controls[n] = value;
}

inline void VehicleInput::set(Logic n, bool value)
{
	if (value)
		logic |= n;
	else
		logic &= ~n;
}

inline void VehicleInput::clear()
{
	for (int i = 0; i < CTRLNUM; ++i)
		controls[i] = 0;
	logic = 0;
}

}

#endif // _SIM_VEHICLEINPUT_H
