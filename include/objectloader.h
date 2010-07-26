#ifndef _OBJECTLOADER_H
#define _OBJECTLOADER_H

#include <string>
#include <ostream>

/// object loader interface
template <typename T>
class ObjectLoader
{
public:
	virtual ~ObjectLoader() {};
	
	/// return NULL on error
	virtual T * load(std::ostream & error) const = 0;
	
	/// unique object id
	virtual const std::string & id() const = 0;
};

#endif // _OBJECTLOADER_H
