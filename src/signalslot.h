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

#ifndef _SIGNALSLOT_H
#define _SIGNALSLOT_H

#include "delegate.h"
#include <vector>

template <typename... Params>
class Signald
{
public:
	void operator()(Params... p) const
	{
		for (const auto & s : m_slots)
			s(p...);
	};
	void connect(Delegated<Params...> s)
	{
		m_slots.push_back(s);
	}
	void disconnect(void)
	{
		m_slots.clear();
	}
	bool connected(void) const
	{
		return !m_slots.empty();
	};

private:
	std::vector<Delegated<Params...>> m_slots;
};

template <typename... Params>
class Slot;

template <typename... Params>
class Signal
{
public:
	Signal(void) {};
	~Signal(void);
	Signal(const Signal & other);
	Signal & operator=(const Signal & other);
	void connect(Slot<Params...> & slot);
	void disconnect(void);
	bool connected(void) const;
	void operator()(Params... p) const;

private:
	friend class Slot<Params...>;
	struct Connection
	{
		Slot<Params...> * slot;
		std::size_t id;
	};
	std::vector<Connection> m_connections;
};

template <typename... Params>
class Slot
{
public:
	Slot(void) {};
	~Slot(void);
	Slot(const Slot & other);
	Slot & operator=(const Slot & other);
	void connect(Signal<Params...> & signal);
	void disconnect(void);
	bool connected(void) const;
	Delegate<void, Params...> call;

private:
	friend class Signal<Params...>;
	struct Connection
	{
		Signal<Params...> * signal;
		std::size_t id;
	};
	std::vector<Connection> m_connections;
};


// Implementation

template <typename... Params>
inline Slot<Params...>::Slot(const Slot<Params...> & other)
{
	*this = other;
}

template <typename... Params>
inline Slot<Params...>::~Slot()
{
	disconnect();
}

template <typename... Params>
inline Slot<Params...> & Slot<Params...>::operator=(const Slot<Params...> & other)
{
	if (this != &other)
	{
		for (auto & con : m_connections)
		{
			connect(*con.signal);
		}
		call = other.call;
	}
	return *this;
}

template <typename... Params>
inline void Slot<Params...>::connect(Signal<Params...> & signal)
{
	typename Signal<Params...>::Connection sg;
	sg.slot = this;
	sg.id = m_connections.size();

	Connection sl;
	sl.signal = &signal;
	sl.id = signal.m_connections.size();

	m_connections.push_back(sl);
	signal.m_connections.push_back(sg);
}

template <typename... Params>
inline void Slot<Params...>::disconnect(void)
{
	for (const auto & con : m_connections)
	{
		// remove slot from signal by swapping
		auto & signal = *con.signal;
		auto id = con.id;
		signal.m_connections[id] = signal.m_connections[signal.m_connections.size() - 1];

		// update id of the swapped connection
		auto & swapped_con = signal.m_connections[id];
		swapped_con.slot->m_connections[swapped_con.id].id = id;

		signal.m_connections.pop_back();
	}
	m_connections.resize(0);
}

template <typename... Params>
inline bool Slot<Params...>::connected(void) const
{
	return m_connections.size();
}

template <typename... Params>
inline Signal<Params...>::Signal(const Signal<Params...> & other)
{
	*this = other;
}

template <typename... Params>
inline Signal<Params...> & Signal<Params...>::operator=(const Signal<Params...> & other)
{
	if (this != &other)
	{
		for (auto & con : other.m_connections)
		{
			con.slot->connect(*this);
		}
	}
	return *this;
}

template <typename... Params>
inline Signal<Params...>::~Signal(void)
{
	disconnect();
}

template <typename... Params>
inline void Signal<Params...>::connect(Slot<Params...> & slot)
{
	slot.connect(*this);
}

template <typename... Params>
inline void Signal<Params...>::disconnect(void)
{
	for (const auto & con : m_connections)
	{
		// remove signal from slot by swapping
		auto & slot = *con.slot;
		auto id = con.id;
		slot.m_connections[id] = slot.m_connections[slot.m_connections.size() - 1];

		// update id of the swapped connection
		auto & swapped_con = slot.m_connections[id];
		swapped_con.signal->m_connections[swapped_con.id].id = id;

		slot.m_connections.pop_back();
	}
	m_connections.resize(0);
}

template <typename... Params>
inline bool Signal<Params...>::connected(void) const
{
	return m_connections.size();
}

template <typename... Params>
inline void Signal<Params...>::operator()(Params... p) const
{
	for (const auto & con : m_connections)
	{
		con.slot->call(p...);
	}
}

#endif //_SIGNALSLOT_H
