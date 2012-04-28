#ifndef _TRIPPLEBUFFER_H
#define _TRIPPLEBUFFER_H

#include <vector>

template <class T>
class TrippleBuffer
{
public:
	TrippleBuffer();

	std::vector<T> & getFirst();

	std::vector<T> & getLast();

	void swapFirst();

	void swapLast();

private:
	std::vector<T> buffer1, buffer2, buffer3;
	std::vector<T> * buffer1p, * buffer2p, * buffer3p;
};


template <class T>
inline TrippleBuffer<T>::TrippleBuffer()
{
	buffer1p = &buffer1;
	buffer2p = &buffer2;
	buffer3p = &buffer3;
}

template <class T>
inline std::vector<T> & TrippleBuffer<T>::getFirst()
{
	return *buffer1p;
}

template <class T>
inline std::vector<T> & TrippleBuffer<T>::getLast()
{
	return *buffer3p;
}

template <class T>
inline void TrippleBuffer<T>::swapFirst()
{
	std::swap(buffer1p, buffer2p);
}

template <class T>
inline void TrippleBuffer<T>::swapLast()
{
	std::swap(buffer2p, buffer3p);
}

#endif
