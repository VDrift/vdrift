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

#include "contentmanager.h"

#include <ostream>

ContentManager::ContentManager(std::ostream & error) :
	error(error)
{
	// ctor
}

ContentManager::~ContentManager()
{
	sweep();
	_logleaks();
}

void ContentManager::addSharedPath(const std::string & path)
{
	sharedpaths.push_back(path);
}

void ContentManager::addPath(const std::string & path)
{
	basepaths.push_back(path);
}

void ContentManager::sweep()
{
	for (auto & cache : factory_cached.m_caches)
	{
		cache->sweep();
	}
}

bool ContentManager::_logleaks()
{
	size_t n = 0;
	for (const auto & cache : factory_cached.m_caches)
	{
		n += cache->size();
	}
	if (n == 0)
		return false;

	error << "Leaked " << n << " cached objects:";
	for (const auto & cache : factory_cached.m_caches)
	{
		error << "\n";
		cache->log(error);
	}
	error << std::endl;
	return false;
}

bool ContentManager::_logerror(
	const std::string & path,
	const std::string & name)
{
	error << "Failed to load \"" << name << "\" from:";
	for (const auto & basepath : basepaths)
	{
		error << "\n" << basepath + '/' + path;
	}
	for (const auto & sharedpath : sharedpaths)
	{
		error << "\n" << sharedpath;
	}
	error << std::endl;
	return false;
}
