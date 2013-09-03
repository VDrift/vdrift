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

#include <fstream>
#include "downloadable.h"

void DownloadableManager::Initialize(const std::string & newfilename)
{
	filename = newfilename;
	Load();
}

std::vector <std::string> DownloadableManager::GetUpdatables(const std::map <std::string, int> & remote_downloadables) const
{
	std::vector <std::string> update;
    
	// Do a naive n*log(n) algorithm for now.
	for (std::map <std::string, int>::const_iterator i = downloadables.begin(); i != downloadables.end(); i++)
	{
		std::map <std::string, int>::const_iterator r = remote_downloadables.find(i->first);
		if (r != remote_downloadables.end() && r->second > i->second)
			update.push_back(i->first);
	}
    
	return update;
}

void DownloadableManager::SetDownloadable(const std::string & name, int new_version)
{
	downloadables[name] = new_version;
	Save();
}

void DownloadableManager::Load()
{
	std::ifstream f(filename.c_str());
	while (f)
	{
		std::string name;
		int local_version(0);
		f >> name;
		if (f && !name.empty())
		{
			f >> local_version;
			if (local_version != 0)
				downloadables[name] = local_version;
		}
	}
}

void DownloadableManager::Save() const
{
	std::ofstream f(filename.c_str());
	for (std::map <std::string, int>::const_iterator i = downloadables.begin(); i != downloadables.end(); i++)
		f << i->first << " " << i->second << std::endl;
}




