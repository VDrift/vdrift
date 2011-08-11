#ifndef _MANAGER_H
#define _MANAGER_H

#include <map>
#include <string>
#include <iostream>
#include <vector>

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

template <class T>
class MANAGER
{
public:
	typedef std::map<const std::string, std::tr1::shared_ptr<T> > container;
	typedef typename container::iterator iterator;
	typedef typename container::const_iterator const_iterator;

	MANAGER(std::ostream & error) : error(error)
	{
		// ctor
	}

	~MANAGER()
	{
		Clear();
	}

	void AddPath(const std::string & path, bool shared = false)
	{
		basepaths.push_back(PATH(path, shared));
	}
	
	bool Get(const std::string & path, const std::string & name, const_iterator & it)
	{
		const_iterator i = objects.find(path + "/" + name);
		if (i == objects.end())
		{
			i = objects.find(name); // shared objects
			if (i == objects.end()) return false;
		}
		it = i;
		return true;
	}

	bool Get(const std::string & path, const std::string & name, std::tr1::shared_ptr<T> & sp)
	{
		const_iterator it;
		if (Get(path, name, it))
		{
			sp = it->second;
			return true;
		}
		return false;
	}

	const_iterator Set(const std::string & path, const std::tr1::shared_ptr<T> & sp)
	{
		return objects.insert(std::pair<std::string, std::tr1::shared_ptr<T> >(path, sp)).first;
	}

	unsigned int Size() const
	{
		return objects.size();
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
			error << "Leak: ";
			DebugPrint(error);
			for (iterator it = objects.begin(); it != objects.end(); it++)
			{
				error << "Leaked: " << it->first << std::endl;
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
	container objects;
	struct PATH
	{
		std::string path;
		bool shared;
		std::string GetAssetPath(const std::string & localpath, const std::string & name) const
		{
			if (shared)
				return name;
			else
				return localpath + "/" + name;
		}
		PATH(const std::string & path_, bool shared_) : path(path_), shared(shared_) {}
	};
	std::vector <PATH> basepaths;
	std::ostream & error;
};

#endif // _MANAGER_H
