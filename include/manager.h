#ifndef _MANAGER_H
#define _MANAGER_H

#include <map>
#include <string>
#include <iostream>
#include <tr1/memory>

/// object T has to have a constructor taking Tinfo and std::ostream & error as parameter
template <class T, class Tinfo>
class MANAGER
{
public:
	MANAGER(std::ostream & error) : 
		error(error), created(0), reused(0), deleted(0)
	{
	}
	
	~MANAGER()
	{
		objectmap.clear();
	}
	
	// get object
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

	// collect garbage
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
	
	void Clear()
	{
		Sweep();
		if (!objectmap.empty())
		{
			error << "Leak: ";
			DebugPrint(error);
			
			for (iterator it = objectmap.begin(); it != objectmap.end(); it++)
			{
				error << "Leaked: " << it->first << std::endl;
			}
		}
	}
	
	void DebugPrint(std::ostream & out)
	{
		int refcount = 0;
		out << "Object manager " << this << " debug print " << std::endl;
		for(iterator it = objectmap.begin(); it != objectmap.end(); it++)
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
	typedef typename std::map<Tinfo, std::tr1::shared_ptr<T> >::iterator iterator;
	std::map<Tinfo, std::tr1::shared_ptr<T> > objectmap;
	std::ostream & error;
	unsigned int created, reused, deleted;
};

#endif // _MANAGER_H
