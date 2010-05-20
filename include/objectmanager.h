#ifndef _OBJECTMANAGER_H
#define _OBJECTMANAGER_H

#include <map>
#include <string>
#include <iostream>
#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

/// object T has to have a constructor taking Tinfo and std::ostream & error as parameter
template < class Tinfo, class T, class Pred = std::less<Tinfo> >
class OBJECTMANAGER
{
public:
	OBJECTMANAGER(std::ostream & error)
	: error(error), created(0), reused(0), deleted(0)
	{
	}
	
	~OBJECTMANAGER()
	{
		objectmap.clear();
	}
	
	// get/create object
	std::tr1::shared_ptr<T> Get(const Tinfo & info)
	{
		iterator it = objectmap.find(info);
		if (it != objectmap.end())
		{
			++reused;
			return it->second;
		}
		++created;
		std::tr1::shared_ptr<T> sp(new T(info, error));
		objectmap[info] = sp;
		return sp;
	}

	// clear expired objects
	void Sweep()
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
	
	void DebugPrint(std::ostream & out)
	{
		out << "Objects count: " << objectmap.size();
		out << ", created: " << created;
		out << ", reused: " << reused;
		out << ", deleted: " << deleted << std::endl;
		created = 0; reused = 0; deleted = 0; // reset counters
	}
	
protected:
	typedef typename std::map<Tinfo, std::tr1::shared_ptr<T>, Pred>::iterator iterator;
	std::map<Tinfo, std::tr1::shared_ptr<T>, Pred> objectmap;
	std::ostream & error;
	unsigned int created, reused, deleted;
};

#endif // _OBJECTMANAGER_H
