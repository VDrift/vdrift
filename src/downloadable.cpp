#include "downloadable.h"

#include <fstream>

void DOWNLOADABLEMANAGER::Initialize(const std::string & newfilename)
{
	filename = newfilename;
	Load();
}

void DOWNLOADABLEMANAGER::Load()
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
			{
				downloadables[name] = local_version;
			}
		}
	}
}

void DOWNLOADABLEMANAGER::Save() const
{
	std::ofstream f(filename.c_str());
	for (std::map <std::string, int>::const_iterator i = downloadables.begin(); i != downloadables.end(); i++)
	{
		f << i->first << " " << i->second << std::endl;
	}
}

void DOWNLOADABLEMANAGER::SetDownloadable(const std::string & name, int new_version)
{
	downloadables[name] = new_version;
	Save();
}

std::vector <std::string> DOWNLOADABLEMANAGER::GetUpdatables(const std::map <std::string, int> & remote_downloadables) const
{
	std::vector <std::string> update;
	
	// do a naive n*log(n) algorithm for now
	for (std::map <std::string, int>::const_iterator i = downloadables.begin(); i != downloadables.end(); i++)
	{
		std::map <std::string, int>::const_iterator r = remote_downloadables.find(i->first);
		if (r != remote_downloadables.end() && r->second > i->second)
			update.push_back(i->first);
	}
	
	return update;
}
