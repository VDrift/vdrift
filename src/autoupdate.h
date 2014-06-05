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

#ifndef _AUTOUPDATE_H
#define _AUTOUPDATE_H

#include "cfg/config.h"

#include <string>
#include <map>

/// A class that maintains version information about cars and tracks.
class AutoUpdate
{
public:
	AutoUpdate();

	/// Write the version information to a VDrift config format file.
	/// Returns true on success and false on error.
	bool Write(const std::string & path) const;

	/// Read the version information from a VDrift config format file.
	/// Returns true on success and false on error.
	bool Load(const std::string & path);

	/// Update folder/version pairs for the specified group in the available_updates map.
	void SetAvailableUpdates(const std::string & group, const std::map <std::string, int> & new_revs);

	/// After SetUpdatesAvailable has been called, return a pair where the first element is the folders that are newer that what we have, and the second is the folders that we have that do not exist in the source.
	std::pair <std::vector <std::string>, std::vector <std::string> > CheckUpdate(const std::string & group) const;

	/// Return the current on disk (first) and latest available (second) revs for the item in the group.
	std::pair <int, int> GetVersions(const std::string & group, const std::string & item) const;

	/// Return available updates of the specified group.
	std::map <std::string, int> GetAvailableUpdates(const std::string & group) const;

	/// Get the format version for the specified group.
	/// Returns zero on error.
	int GetFormatVersion(const std::string & group) const;

	/// Update the local version record for the specified group and item to the specified version.
	void SetVersion(const std::string & group, const std::string & item, int newversion);

	/// Get vdrift data url.
	const std::string & GetFileUrl() const;

	/// Get vdrift svn info url.
	const std::string & GetMetaUrl() const;

	/// Returns true if we have no update data.
	bool empty() const;
	bool empty(const std::string & group) const;

private:
	/// Map from group name to folder/version pairs.
	typedef std::map <std::string, int> PairType;
	typedef std::map <std::string, PairType> GroupType;
	GroupType groups;
	GroupType available_updates;
	PairType formats;
	std::string file_url;
	std::string meta_url;
};

#endif
