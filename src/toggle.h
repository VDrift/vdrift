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

#ifndef _TOGGLE_H
#define _TOGGLE_H

#include <iosfwd>

class Toggle
{
public:
	Toggle();

	void Set(bool nextstate);
	void Set(const Toggle & other);
	bool GetState() const;
	bool GetImpulseRising() const;
	bool GetImpulseFalling() const;
	bool GetImpulse() const;

	void Clear();

	void DebugPrint(std::ostream & out) const;

	void Tick();

private:
	bool state;
	bool laststate;
};

#endif
