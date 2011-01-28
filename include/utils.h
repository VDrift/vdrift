#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

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

};

#endif
