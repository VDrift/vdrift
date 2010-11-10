#ifndef _MANAGER_H
#define _MANAGER_H

#include <map>
#include <string>
#include <iostream>
#include <tr1/memory>

template <class T>
class MANAGER
{
public:
	MANAGER() : error(0)
	{
		// ctor
	}
	
	~MANAGER()
	{
		Clear();
	}
	
	void Init(const std::string & path, std::ostream & error)
	{
		this->path = path;
		this->error = &error;
	}
	
	bool Get(const std::string & name, std::tr1::shared_ptr<T> & sp)
	{
		assert(error);
		iterator it = objects.find(name);
		if (it == objects.end())
		{
			return false;
		}
		sp = it->second;
		return true;
	}
	
	void Set(const std::string & name, const std::tr1::shared_ptr<T> & sp)
	{
		objects[name] = sp;
	}
	
	unsigned int Size() const
	{
		return objects.size();
	}
	
	const std::string & GetPath() const
	{
		return path;
	}

	// collect garbage
	void Sweep()
	{
		iterator it = objects.begin();
		while(it != objects.end())
		{
			if(it->second.unique())
			{
				objects.erase(it++);
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
		if (!objects.empty())
		{
			assert(error);
			*error << "Leak: ";
			DebugPrint(*error);
			for (iterator it = objects.begin(); it != objects.end(); it++)
			{
				*error << "Leaked: " << it->first << std::endl;
			}
		}
	}
	
	void DebugPrint(std::ostream & out)
	{
		int refcount = 0;
		out << "Object manager " << this << " debug print " << std::endl;
		for(iterator it = objects.begin(); it != objects.end(); it++)
		{
			int references = it->second.use_count() - 1;
			refcount += references;
			out << "References: " << references;
			out << " Object: " << it->first << std::endl;
		}
		out << "References count: " << refcount << std::endl;
	}
	
protected:
	typedef typename std::map<const std::string, std::tr1::shared_ptr<T> >::iterator iterator;
	std::map<const std::string, std::tr1::shared_ptr<T> > objects;
	std::string path;
	std::ostream * error;
};

#endif // _MANAGER_H
