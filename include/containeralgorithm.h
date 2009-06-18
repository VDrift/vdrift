#ifndef _CONTAINERALGORITHM_H
#define _CONTAINERALGORITHM_H

//wrap a bunch of the standard library algorithms to work on whole containers

#include <algorithm>
#include <functional>

namespace calgo
{
	template<class Container, class T>
	typename Container::const_iterator find ( const Container & c, const T& value )
	{
		return std::find(c.begin(), c.end(), value);
	}
	
	template<class Container_in, class OutputIterator>
	OutputIterator copy ( const Container_in & c, OutputIterator result )
	{
		return std::copy(c.begin(), c.end(), result);
	}
	
	template<class Container_in>
	void sort ( Container_in & c )
	{
		std::sort(c.begin(), c.end());
	}
	
	template<class Container, class Function>
	Function for_each(Container & c, Function f)
	{
		return std::for_each(c.begin(), c.end(), f);
	}
	
	template <class Container1, class OutputIterator, class UnaryPredicate>
	OutputIterator copy_if(const Container1& container_in, OutputIterator result, UnaryPredicate pred)
	{
		typename Container1::const_iterator i = container_in.begin();
		typename Container1::const_iterator iend = container_in.end();
		for (; i != iend; ++i)
			if (pred( *i ))
				*result++ = *i;
		return result;
	}
	
	template < class Container, class OutputIterator, class UnaryOperator >
	OutputIterator transform ( const Container & c,
		OutputIterator result, UnaryOperator op )
	{
		return std::transform(c.begin(), c.end(), result, op);
	}
	
	template<class Container, class InputIterator, class T>
	InputIterator find ( const Container & c, const T& value )
	{
		return std::find(c.begin(), c.end(), value);
	}
}

#endif
