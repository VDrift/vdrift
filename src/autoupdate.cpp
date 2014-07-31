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
	meta_url("http://sourceforge.net/p/vdrift/code/HEAD/tree/vdrift-data/")
{
	// Constructor
}

bool AutoUpdate::Write(const std::string & path) const
{
	Config conf;

	// Iterate over all groups (cars, tracks).
	// Each group will be a section in the config file.
	for (GroupType::const_iterator g = groups.begin(); g != groups.end(); g++)
	{
		const std::string & section = g->first;

		// Iterate over all paths in this group (360, XS).
		for (PairType::const_iterator p = g->second.begin(); p != g->second.end(); p++)
			conf.set(section, p->first, p->second);
	}

	// Now repeat the above for the available_updates groups.
	for (GroupType::const_iterator g = available_updates.begin(); g != available_updates.end(); g++)
	{
		const std::string & section = AVAILABLE_PREFIX + g->first;

		// Iterate over all paths in this group (360, XS).
		for (PairType::const_iterator p = g->second.begin(); p != g->second.end(); p++)
			conf.set(section, p->first, p->second);
	}

	// Now write formats.
	for (PairType::const_iterator p = formats.begin(); p != formats.end(); p++)
		conf.set("formats", p->first, p->second);

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
	for (Config::const_iterator s = conf.begin(); s != conf.end(); s++)
	{
		// Get the group corresponding to this section (creating it if necessary).
		PairType * group = NULL;
		if (s->first == "formats")
			group = &formats;
		else
		{
			if (s->first.find(AVAILABLE_PREFIX) == 0)
				group = &available_updates[s->first.substr(AVAILABLE_PREFIX.size())];
			else
				group = &groups[s->first];
		}

		// Iterate over all paths in this group.
		for (Config::Section::const_iterator p = s->second.begin(); p != s->second.end(); p++)
		{
			// Convert the configfile string var to an int.
			int revnum(0);
			std::istringstream s(p->second);
			s >> revnum;
			(*group)[p->first] = revnum;
		}
	}

	return true;
}

std::pair <std::vector <std::string>, std::vector <std::string> > AutoUpdate::CheckUpdate(const std::string & group) const
{
	std::vector <std::string> changed;
	std::vector <std::string> deleted;

	// Get the relevant available update group.
	GroupType::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		const PairType & check = availfound->second;

		// Get the relevant group.
		GroupType::const_iterator foundgroup = groups.find(group);

		if (foundgroup == groups.end())
			// Everything is new!
			for (PairType::const_iterator i = check.begin(); i != check.end(); i++)
				changed.push_back(i->first);
		else
		{
			const PairType & cur = foundgroup->second;

			// Use this to keep track of the items in cur that also exist in the input.
			PairType incuronly = cur;

			// Iterate through each input item.
			for (PairType::const_iterator i = check.begin(); i != check.end(); i++)
			{
				// See if we have it; if not, it's new and should be updated.
				PairType::const_iterator c = cur.find(i->first);
				if (c == cur.end())
					changed.push_back(i->first);
				else
					// If we have it, we only need to update if the input rev is newer.
					if (i->second > c->second)
						changed.push_back(i->first);

				// Record that this item exists in the input.
				incuronly.erase(i->first);
			}

			// Now record items that we have that don't exist in the input.
			for (PairType::const_iterator i = incuronly.begin(); i != incuronly.end(); i++)
				deleted.push_back(i->first);
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
	GroupType::const_iterator availfound = available_updates.find(group);
	if (availfound == available_updates.end())
		return true;
	else
		return availfound->second.empty();
}

std::pair <int, int> AutoUpdate::GetVersions(const std::string & group, const std::string & item) const
{
	int local(0), remote(0);

	GroupType::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		PairType::const_iterator remotefound = availfound->second.find(item);
		if (remotefound != availfound->second.end())
			remote = remotefound->second;
	}
	GroupType::const_iterator groupfound = groups.find(group);
	if (groupfound != groups.end())
	{
		PairType::const_iterator localfound = groupfound->second.find(item);
		if (localfound != groupfound->second.end())
			local = localfound->second;
	}

	return std::make_pair(local, remote);
}

std::map <std::string, int> AutoUpdate::GetAvailableUpdates(const std::string & group) const
{
	GroupType::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
		return availfound->second;
	else
		return std::map <std::string, int>();
}

int AutoUpdate::GetFormatVersion(const std::string & group) const
{
	int version = 0;

	PairType::const_iterator i = formats.find(group);
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
