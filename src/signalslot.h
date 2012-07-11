#ifndef _SIGNALSLOT_H
#define _SIGNALSLOT_H

#include "delegate.h"
#include <vector>

template <class Delegate>
class Slot;

template <class Delegate>
class Signal
{
public:
	Signal(void) {};
	Signal(const Signal & other);
	Signal & operator=(const Signal & other);
	void disconnect(void);
	bool connected(void) const;

protected:
	friend class Slot<Delegate>;
	~Signal(void);

	struct Connection
	{
		Slot<Delegate> * slot;
		std::size_t id;
	};
	std::vector<Connection> m_connections;
};

template <class Delegate>
class Slot
{
public:
	Slot(void) {};
	Slot(const Slot & other);
	Slot & operator=(const Slot & other);
	void connect(Signal<Delegate> & signal);
	void disconnect(void);
	Delegate call;

protected:
	friend class Signal<Delegate>;
	~Slot(void);

	struct Connection
	{
		Signal<Delegate> * signal;
		std::size_t id;
	};
	std::vector<Connection> m_connections;
};

class Slot0 : public Slot<Delegate0<void> >
{
	// template typedef here
};

template <typename P>
class Slot1 : public Slot<Delegate1<void, P> >
{
	// template typedef here
};

class Signal0 : public Signal<Delegate0<void> >
{
public:
	Signal0(void);
	Signal0(const Signal0 & other);
	Signal0 & operator=(const Signal0 & other);
	void operator()() const;
};

template <typename P>
class Signal1 : public Signal<Delegate1<void, P> >
{
public:
	Signal1(void);
	Signal1(const Signal1 & other);
	Signal1 & operator=(const Signal1 & other);
	void operator()(P p) const;
};

// Implementation

template <class Delegate>
inline Slot<Delegate>::Slot(const Slot & other)
{
	*this = other;
}

template <class Delegate>
inline Slot<Delegate>::~Slot()
{
	disconnect();
}

template <class Delegate>
inline Slot<Delegate> & Slot<Delegate>::operator=(const Slot<Delegate> & other)
{
	for (std::size_t i = 0; i < m_connections.size(); ++i)
	{
		connect(*m_connections[i].signal);
	}
	call = other.call;
	return *this;
}

template <class Delegate>
inline void Slot<Delegate>::connect(Signal<Delegate> & signal)
{
	typename Signal<Delegate>::Connection sg;
	sg.slot = this;
	sg.id = m_connections.size();

	Connection sl;
	sl.signal = &signal;
	sl.id = signal.m_connections.size();

	m_connections.push_back(sl);
	signal.m_connections.push_back(sg);
}

template <class Delegate>
inline void Slot<Delegate>::disconnect(void)
{
	for (std::size_t i = 0; i < m_connections.size(); ++i)
	{
		// remove slot from signal by swapping
		Signal<Delegate> & signal = *m_connections[i].signal;
		std::size_t id = m_connections[i].id;
		signal.m_connections[id] = signal.m_connections[signal.m_connections.size() - 1];

		// update id of the swapped connection
		typename Signal<Delegate>::Connection & con = signal.m_connections[id];
		con.slot->m_connections[con.id].id = id;
		signal.m_connections.pop_back();
	}
	m_connections.resize(0);
}

template <class Delegate>
inline Signal<Delegate>::Signal(const Signal & other)
{
	*this = other;
}

template <class Delegate>
inline Signal<Delegate> & Signal<Delegate>::operator=(const Signal & other)
{
	for (std::size_t i = 0; i < m_connections.size(); ++i)
	{
		m_connections[i].slot->connect(*this);
	}
	return *this;
}

template <class Delegate>
inline Signal<Delegate>::~Signal(void)
{
	disconnect();
}

template <class Delegate>
inline void Signal<Delegate>::disconnect(void)
{
	for (std::size_t i = 0; i < m_connections.size(); ++i)
	{
		// remove signal from slot by swapping
		Slot<Delegate> & slot = *m_connections[i].slot;
		std::size_t id = m_connections[i].id;
		slot.m_connections[id] = slot.m_connections[slot.m_connections.size() - 1];

		// update id of the swapped connection
		typename Slot<Delegate>::Connection & con = slot.m_connections[id];
		con.signal->m_connections[con.id].id = id;
		slot.m_connections.pop_back();
	}
	m_connections.resize(0);
}

template <class Delegate>
inline bool Signal<Delegate>::connected(void) const
{
	return m_connections.size();
}

inline Signal0::Signal0(void)
{
	// ctor
}

inline Signal0::Signal0(const Signal0 & other)
{
	*this = other;
}

inline Signal0 & Signal0::operator=(const Signal0 & other)
{
	Signal<Delegate0<void> >::operator=(other);
	return *this;
}

inline void Signal0::operator()() const
{
	for (std::size_t i = 0; i < m_connections.size(); ++i)
	{
		m_connections[i].slot->call();
	}
}

template <typename P>
inline Signal1<P>::Signal1(void)
{
	// ctor
}

template <typename P>
inline Signal1<P>::Signal1(const Signal1 & other)
{
	*this = other;
}

template <typename P>
inline Signal1<P> & Signal1<P>::operator=(const Signal1 & other)
{
	Signal<Delegate1<void, P> >::operator=(other);
	return *this;
}

template <typename P>
inline void Signal1<P>::operator()(P p) const
{
	for (std::size_t i = 0; i < Signal<Delegate1<void, P> >::m_connections.size(); ++i)
	{
		Signal<Delegate1<void, P> >::m_connections[i].slot->call(p);
	}
}

#endif //_SIGNALSLOT_H
