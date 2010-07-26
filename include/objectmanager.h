#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include "objectloader.h"

#include <map>
#include <string>
#include <iostream>
#include <cassert>

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

/// shared object manager
template <typename T>
class ObjectManager
{
public:
	ObjectManager(std::ostream & error) :
		error(error), created(0), reused(0), deleted(0)
	{
	}

	~ObjectManager()
	{
	}

	std::tr1::shared_ptr<T> get(const ObjectLoader<T> & loader)
	{
		const std::string & id = loader.id();
		
		iterator it = objectmap.find(id);
		if (it != objectmap.end())
		{
			++reused;
			return it->second;
		}
		++created;
		
		T * object = loader.load(error);
		if (!object)
		{
			return std::tr1::shared_ptr<T>();
		}

		std::tr1::shared_ptr<T> ptr(object);
		objectmap[id] = ptr;
		return ptr;
	}

	// clear expired objects
	void sweep()
	{
		iterator it = objectmap.begin();
		while(it != objectmap.end())
		{
			if(it->second.unique())
			{
				++deleted;
				objectmap.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	void clear()
	{
		sweep();
		if (!objectmap.empty())
		{
			error << "Leak: ";
			debugPrint(error);
			
			for (iterator it = objectmap.begin(); it != objectmap.end(); ++it)
			{
				error << "Leaked: " << it->first << std::endl;
			}
		}
	}

	void debugPrint(std::ostream & out)
	{
		int refcount = 0;
		out << "Object manager " << this << " debug print " << std::endl;
		for(iterator it = objectmap.begin(); it != objectmap.end(); ++it)
		{
			int references = it->second.use_count() - 1; // subtract our reference
			refcount += references;
			out << "References: " << references;
			out << " Object: " << it->first << std::endl;
		}
		out << "References count: " << refcount << std::endl;
		
		out << "Objects count: " << objectmap.size();
		out << ", created: " << created;
		out << ", reused: " << reused;
		out << ", deleted: " << deleted << std::endl;
		created = 0; reused = 0; deleted = 0; // reset counters
	}

protected:
	typedef typename std::map<std::string, std::tr1::shared_ptr<T> >::iterator iterator;
	std::map<std::string,  std::tr1::shared_ptr<T> > objectmap;
	std::ostream & error;
	unsigned int created, reused, deleted;
};

#endif // _OBJECTMANAGER_H
