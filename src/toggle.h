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
	void Tick();

	template <class Stream>
	void DebugPrint(Stream & out) const;

private:
	bool state;
	bool laststate;
};


inline Toggle::Toggle()
{
	Clear();
}

inline void Toggle::Set(bool nextstate)
{
	state = nextstate;
}

inline void Toggle::Set(const Toggle & other)
{
	state = other.GetState();
	laststate = (!state && other.GetImpulseFalling()) || (state && !other.GetImpulseRising());
}

inline bool Toggle::GetState() const
{
	return state;
}

inline bool Toggle::GetImpulseRising() const
{
	return (state && !laststate);
}

inline bool Toggle::GetImpulseFalling() const
{
	return (!state && laststate);
}

inline bool Toggle::GetImpulse() const
{
	return (GetImpulseRising() || GetImpulseFalling());
}

inline void Toggle::Clear()
{
	state = false;
	laststate = false;
}

inline void Toggle::Tick()
{
	laststate = state;
}

template <class Stream>
inline void Toggle::DebugPrint(Stream & out) const
{
	out << "State: " << state << "  Last state: " << laststate << "\n";
}

#endif
