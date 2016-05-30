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

#include "autoupdate.h"
#include "cfg/config.h"
#include <sstream>

const std::string AVAILABLE_PREFIX = "available_";

AutoUpdate::AutoUpdate() :
	file_url("http://svn.code.sf.net/p/vdrift/code/vdrift-data/"),
	meta_url("https://sourceforge.net/p/vdrift/code/HEAD/tree/vdrift-data/")
{
	// Constructor
}

bool AutoUpdate::Write(const std::string & path) const
{
	Config conf;

	// Iterate over all groups (cars, tracks).
	// Each group will be a section in the config file.
	for (const auto & group : groups)
	{
		const std::string & section = group.first;

		// Iterate over all paths in this group (360, XS).
		for (const auto & p : group.second)
			conf.set(section, p.first, p.second);
	}

	// Now repeat the above for the available_updates groups.
	for (const auto & update : available_updates)
	{
		const std::string & section = AVAILABLE_PREFIX + update.first;

		// Iterate over all paths in this group (360, XS).
		for (const auto & p : update.second)
			conf.set(section, p.first, p.second);
	}

	// Now write formats.
	for (const auto & format : formats)
		conf.set("formats", format.first, format.second);

	// Write data url.
	conf.set("", "meta_url", meta_url);
	conf.set("", "file_url", file_url);

	// Write to disk.
	return conf.write(path);
}

bool AutoUpdate::Load(const std::string & path)
{
	Config conf;

	if (!conf.load(path))
		return false;

	// Clear the existing data.
	groups.clear();

	// Get data url
	conf.get("", "meta_url", meta_url);
	conf.get("", "file_url", file_url);

	// Iterate over all sections.
	for (const auto & s : conf)
	{
		// Get the group corresponding to this section (creating it if necessary).
		PairType * group = NULL;
		if (s.first == "formats")
			group = &formats;
		else
		{
			if (s.first.find(AVAILABLE_PREFIX) == 0)
				group = &available_updates[s.first.substr(AVAILABLE_PREFIX.size())];
			else
				group = &groups[s.first];
		}

		// Iterate over all paths in this group.
		for (const auto & p : s.second)
		{
			// Convert the configfile string var to an int.
			int revnum(0);
			std::istringstream s(p.second);
			s >> revnum;
			(*group)[p.first] = revnum;
		}
	}

	return true;
}

std::pair <std::vector <std::string>, std::vector <std::string> > AutoUpdate::CheckUpdate(const std::string & group) const
{
	std::vector <std::string> changed;
	std::vector <std::string> deleted;

	// Get the relevant available update group.
	auto availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		const PairType & check = availfound->second;

		// Get the relevant group.
		auto foundgroup = groups.find(group);

		if (foundgroup == groups.end())
			// Everything is new!
			for (const auto & item : check)
				changed.push_back(item.first);
		else
		{
			const PairType & cur = foundgroup->second;

			// Use this to keep track of the items in cur that also exist in the input.
			PairType incuronly = cur;

			// Iterate through each input item.
			for (const auto & item : check)
			{
				// See if we have it; if not, it's new and should be updated.
				auto c = cur.find(item.first);
				if (c == cur.end())
					changed.push_back(item.first);
				else
					// If we have it, we only need to update if the input rev is newer.
					if (item.second > c->second)
						changed.push_back(item.first);

				// Record that this item exists in the input.
				incuronly.erase(item.first);
			}

			// Now record items that we have that don't exist in the input.
			for (const auto & item : incuronly)
				deleted.push_back(item.first);
		}
	}

	return std::make_pair(changed, deleted);
}

void AutoUpdate::SetAvailableUpdates(const std::string & group, const std::map <std::string, int> & new_revs)
{
	available_updates[group] = new_revs;
}

bool AutoUpdate::empty() const
{
	return groups.empty();
}

bool AutoUpdate::empty(const std::string & group) const
{
	auto availfound = available_updates.find(group);
	if (availfound == available_updates.end())
		return true;
	else
		return availfound->second.empty();
}

std::pair <int, int> AutoUpdate::GetVersions(const std::string & group, const std::string & item) const
{
	int local(0), remote(0);

	auto availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		auto remotefound = availfound->second.find(item);
		if (remotefound != availfound->second.end())
			remote = remotefound->second;
	}
	auto groupfound = groups.find(group);
	if (groupfound != groups.end())
	{
		auto localfound = groupfound->second.find(item);
		if (localfound != groupfound->second.end())
			local = localfound->second;
	}

	return std::make_pair(local, remote);
}

std::map <std::string, int> AutoUpdate::GetAvailableUpdates(const std::string & group) const
{
	auto availfound = available_updates.find(group);
	if (availfound != available_updates.end())
		return availfound->second;
	else
		return std::map <std::string, int>();
}

int AutoUpdate::GetFormatVersion(const std::string & group) const
{
	int version = 0;

	auto i = formats.find(group);
	if (i != formats.end())
		version = i->second;

	return version;
}

void AutoUpdate::SetVersion(const std::string & group, const std::string & item, int newversion)
{
	groups[group][item] = newversion;
}

const std::string & AutoUpdate::GetFileUrl() const
{
	return file_url;
}

const std::string & AutoUpdate::GetMetaUrl() const
{
	return meta_url;
}
