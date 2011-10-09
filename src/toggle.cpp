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

#include "toggle.h"
#include "unittest.h"

using std::endl;

TOGGLE::TOGGLE()
{
	Clear();
}

void TOGGLE::Set(bool nextstate)
{
	state = nextstate;
}

void TOGGLE::Set(const TOGGLE & other)
{
	state = other.GetState();
	laststate = (!state && other.GetImpulseFalling()) || (state && !other.GetImpulseRising());
}

bool TOGGLE::GetState() const
{
	return state;
}

bool TOGGLE::GetImpulseRising() const
{
	return (state && !laststate);
}

bool TOGGLE::GetImpulseFalling() const
{
	return (!state && laststate);
}

bool TOGGLE::GetImpulse() const
{
	return (GetImpulseRising() || GetImpulseFalling());
}

void TOGGLE::Clear()
{
	state = false;
	laststate = false;
}

void TOGGLE::DebugPrint(std::ostream & out) const
{
	out << "State: " << state << "  Last state: " << laststate << endl;
}

void TOGGLE::Tick()
{
	laststate = state;
}

QT_TEST(toggle_test)
{
	TOGGLE t;
	t.Clear();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Tick();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Set(false);
	t.Tick();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Set(true);
	QT_CHECK(t.GetState() && t.GetImpulse() && t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Tick();
	QT_CHECK(t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Tick();
	QT_CHECK(t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Set(false);
	QT_CHECK(!t.GetState() && t.GetImpulse() && !t.GetImpulseRising() && t.GetImpulseFalling());
	t.Tick();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Tick();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
	t.Set(true);
	t.Tick();
	t.Clear();
	QT_CHECK(!t.GetState() && !t.GetImpulse() && !t.GetImpulseRising() && !t.GetImpulseFalling());
}
