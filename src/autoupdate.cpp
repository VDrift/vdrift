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

#include <sstream>
#include "autoupdate.h"
#include "config.h"

const std::string AVAILABLE_PREFIX = "available_";

AUTOUPDATE::AUTOUPDATE() :
	url("http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/")
{
	// Constructor
}

bool AUTOUPDATE::Write(const std::string & path) const
{
	CONFIG conf;

	// Iterate over all groups (cars, tracks).
	// Each group will be a section in the config file.
	for (group_type::const_iterator g = groups.begin(); g != groups.end(); g++)
	{
		const std::string & section = g->first;

		// Iterate over all paths in this group (360, XS).
		for (pair_type::const_iterator p = g->second.begin(); p != g->second.end(); p++)
			conf.SetParam(section, p->first, p->second);
	}

	// Now repeat the above for the available_updates groups.
	for (group_type::const_iterator g = available_updates.begin(); g != available_updates.end(); g++)
	{
		const std::string & section = AVAILABLE_PREFIX + g->first;

		// Iterate over all paths in this group (360, XS).
		for (pair_type::const_iterator p = g->second.begin(); p != g->second.end(); p++)
			conf.SetParam(section, p->first, p->second);
	}

	// Now write formats.
	for (pair_type::const_iterator p = formats.begin(); p != formats.end(); p++)
		conf.SetParam("formats", p->first, p->second);

	// Write data url.
	conf.SetParam("", "url", url);

	// Write to disk.
	return conf.Write(path);
}

bool AUTOUPDATE::Load(const std::string & path)
{
	CONFIG conf;

	if (!conf.Load(path))
		return false;

	// Clear the existing data.
	groups.clear();

	// Get data url
	conf.GetParam("", "url", url);

	// Iterate over all sections.
	for (CONFIG::const_iterator s = conf.begin(); s != conf.end(); s++)
	{
		// Get the group corresponding to this section (creating it if necessary).
		pair_type * group = NULL;
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
		for (CONFIG::SECTION::const_iterator p = s->second.begin(); p != s->second.end(); p++)
		{
			// Convert the configfile string var to an int.
			int revnum(0);
			std::stringstream s(p->second);
			s >> revnum;
			(*group)[p->first] = revnum;
		}
	}

	return true;
}

std::pair <std::vector <std::string>, std::vector <std::string> > AUTOUPDATE::CheckUpdate(const std::string & group) const
{
	std::vector <std::string> changed;
	std::vector <std::string> deleted;

	// Get the relevant available update group.
	group_type::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		const pair_type & check = availfound->second;

		// Get the relevant group.
		group_type::const_iterator foundgroup = groups.find(group);

		if (foundgroup == groups.end())
			// Everything is new!
			for (pair_type::const_iterator i = check.begin(); i != check.end(); i++)
				changed.push_back(i->first);
		else
		{
			const pair_type & cur = foundgroup->second;

			// Use this to keep track of the items in cur that also exist in the input.
			pair_type incuronly = cur;

			// Iterate through each input item.
			for (pair_type::const_iterator i = check.begin(); i != check.end(); i++)
			{
				// See if we have it; if not, it's new and should be updated.
				pair_type::const_iterator c = cur.find(i->first);
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
			for (pair_type::const_iterator i = incuronly.begin(); i != incuronly.end(); i++)
				deleted.push_back(i->first);
		}
	}

	return std::make_pair(changed, deleted);
}

void AUTOUPDATE::SetAvailableUpdates(const std::string & group, const std::map <std::string, int> & new_revs)
{
	available_updates[group] = new_revs;
}

bool AUTOUPDATE::empty() const
{
	return groups.empty();
}

bool AUTOUPDATE::empty(const std::string & group) const
{
	group_type::const_iterator availfound = available_updates.find(group);
	if (availfound == available_updates.end())
		return true;
	else
		return availfound->second.empty();
}

std::pair <int, int> AUTOUPDATE::GetVersions(const std::string & group, const std::string & item) const
{
	int local(0), remote(0);

	group_type::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
	{
		pair_type::const_iterator remotefound = availfound->second.find(item);
		if (remotefound != availfound->second.end())
			remote = remotefound->second;
	}
	group_type::const_iterator groupfound = groups.find(group);
	if (groupfound != groups.end())
	{
		pair_type::const_iterator localfound = groupfound->second.find(item);
		if (localfound != groupfound->second.end())
			local = localfound->second;
	}

	return std::make_pair(local, remote);
}

std::map <std::string, int> AUTOUPDATE::GetAvailableUpdates(const std::string & group) const
{
	group_type::const_iterator availfound = available_updates.find(group);
	if (availfound != available_updates.end())
		return availfound->second;
	else
		return std::map <std::string, int>();
}

int AUTOUPDATE::GetFormatVersion(const std::string & group) const
{
	int version = 0;

	pair_type::const_iterator i = formats.find(group);
	if (i != formats.end())
		version = i->second;

	return version;
}

void AUTOUPDATE::SetVersion(const std::string & group, const std::string & item, int newversion)
{
	groups[group][item] = newversion;
}

const std::string & AUTOUPDATE::GetUrl() const
{
	return url;
}
