#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include <vector>

namespace UTILS
{

///seeks to the token and returns the text minus the token
/// if we got to the end of the file, return what we have so far
std::string SeekTo(std::istream & in, const std::string & token);

std::string LoadFileIntoString(const std::string & filepath, std::ostream & error_output);

template <typename T>
std::string tostr(T val)
{
	std::stringstream s;
	s << val;
	return s.str();
}

template <typename T>
T fromstr(const std::string & str)
{
	T val;
	std::stringstream s(str);
	s >> val;
	return val;
}

/// erase the index from the vector by swapping it with the last entry and popping the last entry
template <typename T>
void eraseVectorUseSwapAndPop(unsigned int index, T & vector)
{
	// note that an empty vector will cause this assert to fail
	assert(index < vector.size());

	// see if we actually need to swap
	unsigned int last = vector.size()-1;
	if (index != last)
	{
		// yes, let's swap
		std::swap(vector[index],vector[last]);
	}

	// pop
	vector.pop_back();
}

// works if T is an STL (or STL-like) container
template <typename T>
std::string implode(const T & toImplode, const std::string & sep)
{
	std::string out;

	for (typename T::const_iterator i = toImplode.begin(); i != toImplode.end(); i++)
	{
		if (i != toImplode.begin())
			out.append(sep);
		out.append(*i);
	}

	return out;
}

std::vector <std::string> explode(const std::string & toExplode, const std::string & sep);

/// print all elements in the vector to the provided ostream
template <typename T>
void print_vector(const std::vector <T> & v, std::ostream & o, const std::string delim = ", ")
{
	for (typename std::vector <T>::const_iterator i = v.begin(); i != v.end(); i++)
	{
		if (i != v.begin())
			o << delim;
		o << *i;
	}
}

}

#endif
