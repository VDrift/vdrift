#ifdef __APPLE__
	#include <boost/tr1/unordered_map.hpp>
	#include <boost/tr1/functional.hpp>
#elif _MSC_VER
	#include <unordered_map>
#else
	#include <tr1/unordered_map>
#endif
