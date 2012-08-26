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

#include <cassert>

template <class T>
class TrippleBuffer
{
public:
	TrippleBuffer();

	T & getFirst();
	
	T & getSecond();
	
	T & getLast();

	void swapFirst();

	void swapLast();

private:
	T buffer1, buffer2, buffer3;
	T * buffer1p, * buffer2p, * buffer3p;
};


template <class T>
inline TrippleBuffer<T>::TrippleBuffer()
{
	buffer1p = &buffer1;
	buffer2p = &buffer2;
	buffer3p = &buffer3;
}

template <class T>
inline T & TrippleBuffer<T>::getFirst()
{
	return *buffer1p;
}

template <class T>
inline T & TrippleBuffer<T>::getSecond()
{
	return *buffer2p;
}

template <class T>
inline T & TrippleBuffer<T>::getLast()
{
	return *buffer3p;
}

template <class T>
inline void TrippleBuffer<T>::swapFirst()
{
	std::swap(buffer1p, buffer2p);
	assert(buffer1p != buffer2p);
	assert(buffer2p != buffer3p);
	assert(buffer3p != buffer1p);

}

template <class T>
inline void TrippleBuffer<T>::swapLast()
{
	std::swap(buffer2p, buffer3p);
	assert(buffer1p != buffer2p);
	assert(buffer2p != buffer3p);
	assert(buffer3p != buffer1p);
}

#endif
