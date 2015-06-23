/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _CONTENTMANAGER_H
#define _CONTENTMANAGER_H

#include "soundfactory.h"
#include "texturefactory.h"
#include "modelfactory.h"
#include "configfactory.h"
#include <vector>
#include <map>

class ContentManager
{
public:
	ContentManager(std::ostream & error);

	~ContentManager();

	/// retrieve shared object if in cache
	template <class T>
	bool get(
		std::shared_ptr<T> & sptr,
		const std::string & path,
		const std::string & name);

	/// retrieve shared object, load if not in cache
	template <class T>
	bool load(
		std::shared_ptr<T> & sptr,
		const std::string & path,
		const std::string & name);

	/// support additional optional parameters
	template <class T, class P>
	bool load(
		std::shared_ptr<T> & sptr,
		const std::string & path,
		const std::string & name,
		const P & param);

	/// add shared content directory path
	void addSharedPath(const std::string & path);

	/// add content directory path
	void addPath(const std::string & path);

	/// garbage collect unused content
	void sweep();

	/// factories access
	template <class T>
	Factory<T> & getFactory();

private:
	struct Cache
	{
		virtual void log(std::ostream & log) const = 0;
		virtual size_t size() const = 0;
		virtual void sweep() = 0;
	};

	template <class T>
	class CacheShared : public Cache, public std::map<std::string, std::shared_ptr<T> >
	{
		void log(std::ostream & log) const;
		size_t size() const;
		void sweep();
	};

	/// register content factories
	/// sweep(garbage collection) is mandatory
	struct FactoryCached
	{
		std::vector<Cache*> m_caches;

		#define REGISTER(T)\
		Factory<T> T ## _factory;\
		CacheShared<T> T ## _cache;\
		operator Factory<T>&() {return T ## _factory;}\
		operator CacheShared<T>&() {return T ## _cache;}
		REGISTER(SoundBuffer)
		REGISTER(Texture)
		REGISTER(Model)
		REGISTER(PTree)
		#undef REGISTER

		FactoryCached()
		{
			#define INIT(T) m_caches.push_back(&T ## _cache);
			INIT(SoundBuffer)
			INIT(Texture)
			INIT(Model)
			INIT(PTree)
			#undef INIT
		}

	} factory_cached;

	/// content paths
	std::vector<std::string> sharedpaths;
	std::vector<std::string> basepaths;

	/// error log
	std::ostream & error;

	/// content leak logger
	bool _logleaks();

	/// error logger
	bool _logerror(
		const std::string & path,
		const std::string & name);

	/// get implementation
	template <class T>
	bool _get(
		std::shared_ptr<T> & sptr,
		const std::string & name);

	/// load implementation
	template <class T, class P>
	bool _load(
		std::shared_ptr<T> & sptr,
		const std::vector<std::string> & basepaths,
		const std::string & relpath,
		const std::string & name,
		const P & param);

	/// get default object instance
	template <class T>
	bool _getdefault(std::shared_ptr<T> & sptr);
};

template <class T>
inline bool ContentManager::get(
	std::shared_ptr<T> & sptr,
	const std::string & path,
	const std::string & name)
{
	// check for the specialised version
	// fall back to the generic one
	return 	_get(sptr, path + name) ||
			_get(sptr, name);
}

template <class T>
inline bool ContentManager::load(
	std::shared_ptr<T> & sptr,
	const std::string & path,
	const std::string & name)
{
	return load(sptr, path, name, typename Factory<T>::empty());
}

template <class T, class P>
inline bool ContentManager::load(
	std::shared_ptr<T> & sptr,
	const std::string & path,
	const std::string & name,
	const P & param)
{
	// check for the specialised version in basepaths
	// fall back to the generic one in shared paths
	return 	_load(sptr, basepaths, path, name, param) ||
			_load(sptr, sharedpaths, "", name, param) ||
			_getdefault(sptr) ||
			_logerror(path, name);
}

template <class T>
inline bool ContentManager::_get(
	std::shared_ptr<T> & sptr,
	const std::string & name)
{
	// retrieve from cache
	CacheShared<T> & cache = factory_cached;
	typename CacheShared<T>::const_iterator i = cache.find(name);
	if (i != cache.end())
	{
		sptr = i->second;
		return true;
	}
	return false;
}

template <class T, class P>
inline bool ContentManager::_load(
	std::shared_ptr<T> & sptr,
	const std::vector<std::string> & basepaths,
	const std::string & relpath,
	const std::string & name,
	const P & param)
{
	// check cache
	if (_get(sptr, relpath + name))
	{
		return true;
	}

	// load from basepaths
	Factory<T>& factory = getFactory<T>();
	for (size_t i = 0; i < basepaths.size(); ++i)
	{
		if (factory.create(sptr, error, basepaths[i], relpath, name, param))
		{
			// cache loaded content
			CacheShared<T> & cache = factory_cached;
			cache[relpath + name] = sptr;
			return true;
		}
	}

	return false;
}

template <class T>
inline bool ContentManager::_getdefault(std::shared_ptr<T> & sptr)
{
	sptr = Factory<T>(factory_cached).getDefault();
	return false;
}

template <class T>
inline void ContentManager::CacheShared<T>::log(std::ostream & log) const
{
	typename CacheShared<T>::const_iterator it = CacheShared<T>::begin();
	while (it != CacheShared<T>::end())
	{
		log << it->second.use_count() << " : " << it->first;
	}
}

template <class T>
inline size_t ContentManager::CacheShared<T>::size() const
{
	return std::map<std::string, std::shared_ptr<T> >::size();
}

template <class T>
inline void ContentManager::CacheShared<T>::sweep()
{
	typename CacheShared<T>::iterator it = CacheShared<T>::begin();
	while (it != CacheShared<T>::end())
	{
		if (it->second.unique())
			CacheShared<T>::erase(it++);
		else
			++it;
	}
}

template <class T>
inline Factory<T> & ContentManager::getFactory()
{
	return factory_cached;
}

#endif // _CONTENTMANAGER_H
