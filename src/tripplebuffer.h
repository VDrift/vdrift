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

#ifndef _TRIPPLEBUFFER_H
#define _TRIPPLEBUFFER_H

#include <atomic>

template <class T>
class TrippleBuffer
{
public:
	TrippleBuffer();

	// consumer interface

	// get buffer for processing
	T & front();

	// get next buffer for processing
	bool swap_front();


	// producer interface

	// get buffer for modification
	T & back();

	// commit modified buffer
	bool swap_back();

private:
	struct alignas(64) { T data; } buffer[3];

	// consumer writes head
	alignas(64) std::atomic<unsigned> head;

	// producer writes tail
	alignas(64) std::atomic<unsigned> tail;
};


template <class T>
inline TrippleBuffer<T>::TrippleBuffer() : head(0), tail(1)
{
	// ctor
}

template <class T>
inline T & TrippleBuffer<T>::front()
{
	return buffer[head.load(std::memory_order_relaxed)].data;
}

template <class T>
inline bool TrippleBuffer<T>::swap_front()
{
	auto head_next = (head.load(std::memory_order_relaxed) + 1) % 3;
	if (tail.load(std::memory_order_acquire) != head_next) {
		head.store(head_next, std::memory_order_release);
		return true;
	}
	return false;
}

template <class T>
inline T & TrippleBuffer<T>::back()
{
	return buffer[tail.load(std::memory_order_relaxed)].data;
}

template <class T>
inline bool TrippleBuffer<T>::swap_back()
{
	auto tail_next = (tail.load(std::memory_order_relaxed) + 1) % 3;
	if (head.load(std::memory_order_acquire) != tail_next) {
		tail.store(tail_next, std::memory_order_release);
		return true;
	}
	return false;
}

#endif
