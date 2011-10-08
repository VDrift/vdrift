#ifndef _CONTENTMANAGER_H
#define _CONTENTMANAGER_H

#include "soundinfo.h"

#include <map>
#include <string>
#include <vector>
#include <iostream>

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

class SOUNDBUFFER;
class TEXTURE;
class MODEL;

class ContentManager
{
public:
	ContentManager(std::ostream & error);

	~ContentManager();

	template <class T>
	bool get(const std::string & path, const std::string & name, std::tr1::shared_ptr<T>& sptr);

	template <class T>
	bool load(const std::string & path, const std::string & name, std::tr1::shared_ptr<T>& sptr);

	template <class T, class P>
	bool load(const std::string & path, const std::string & name, const P& param, std::tr1::shared_ptr<T>& sptr);

	/// shared content directory path
	void addSharedPath(const std::string & path);

	/// content directory path
	void addPath(const std::string & path);

	/// sound device setting
	void setSound(const SOUNDINFO& info);

	/// textures size setting
	void setTexSize(const std::string& value);

	/// in general all textures on disk will be in the SRGB colorspace, so if the renderer wants to do
	/// gamma correct lighting, it will want all textures to be gamma corrected using the SRGB flag
	void setSRGB(bool value);

	/// use VBOs instead of draw lists for models
	void setVBO(bool value);

	void sweep();

private:
	template <class T>
	class Cache : public std::map<std::string, std::tr1::shared_ptr<T> >
	{
	public:
		void sweep();
	};

	// content caches
	Cache<SOUNDBUFFER> sounds;
	Cache<TEXTURE> textures;
	Cache<MODEL> models;

	// content settings
	SOUNDINFO sound_info;
	std::string texture_size;
	bool texture_srgb;
	bool model_vbo;

	// content paths
	std::vector<std::string> sharedpaths;
	std::vector<std::string> basepaths;
	std::ostream & error;

	struct empty {};

	template <class T>
	Cache<T>& getCache();

	template <class T, class P>
	bool load(
		std::tr1::shared_ptr<T>& sptr,
		const std::string& abspath,
		const P& param);

	template <class T, class P>
	bool load(
		std::tr1::shared_ptr<T>& sptr,
		const std::vector<std::string>& paths,
		const std::string& relpath,
		const P& param);
};

template <>
inline ContentManager::Cache<TEXTURE>& ContentManager::getCache()
{
	return textures;
}

template <>
inline ContentManager::Cache<MODEL>& ContentManager::getCache()
{
	return models;
}

template <>
inline ContentManager::Cache<SOUNDBUFFER>& ContentManager::getCache()
{
	return sounds;
}

template <class T>
inline void ContentManager::Cache<T>::sweep()
{
	typename Cache::iterator it = this->begin();
	while (it != this->end())
	{
		if (it->second.unique())
		{
			this->erase(it++);
		}
		else
		{
			++it;
		}
	}
}

template <class T, class P>
inline bool ContentManager::load(
	std::tr1::shared_ptr<T>& sptr,
	const std::vector<std::string>& paths,
	const std::string& relpath,
	const P& param)
{
	Cache<T>& cache = getCache<T>();
	typename Cache<T>::const_iterator i = cache.find(relpath);
	if (i != cache.end())
	{
		sptr = i->second;
		return true;
	}
	for (size_t i = 0; i < paths.size(); ++i)
	{
		if (load(sptr, paths[i] + '/' + relpath, param))
		{
			cache[relpath] = sptr;
			return true;
		}
	}
	return false;
}

template <class T, class P>
inline bool ContentManager::load(
	const std::string & path,
	const std::string & name,
	const P& param,
	std::tr1::shared_ptr<T>& sptr)
{
	if (!path.empty())
	{
		if (load(sptr, basepaths, path + '/' + name, param) ||
			load(sptr, sharedpaths, name, param))
		{
			return true;
		}
	}
	else
	{
		if (load(sptr, basepaths, name, param) ||
			load(sptr, sharedpaths, name, param))
		{
			return true;
		}
	}
	error << "Failed to load " << name << " from:";
	for (size_t i = 0; i < basepaths.size(); ++i)
	{
		error << " " << basepaths[i] + '/' + path;
	}
	for (size_t i = 0; i < sharedpaths.size(); ++i)
	{
		error << " " << sharedpaths[i];
	}
	error << std::endl;
	return false;
}

template <class T>
inline bool ContentManager::load(
	const std::string & path,
	const std::string & name,
	std::tr1::shared_ptr<T>& sptr)
{
	return load(path, name, empty(), sptr);
}

template <class T>
inline bool ContentManager::get(
	const std::string & path,
	const std::string & name,
	std::tr1::shared_ptr<T>& sptr)
{
	Cache<T>& cache = getCache<T>();
	typename Cache<T>::const_iterator i;
	if (!path.empty())
	{
		i = cache.find(path + '/' + name);
		if (i != cache.end())
		{
			sptr = i->second;
			return true;
		}
	}
	i = cache.find(name);
	if (i != cache.end())
	{
		sptr = i->second;
		return true;
	}
	return false;
}

#endif // _CONTENTMANAGER_H
