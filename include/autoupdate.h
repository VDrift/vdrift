#ifndef _AUTOUPDATE_H
#define _AUTOUPDATE_H

#include <string>
#include <map>

#include "config.h"

/// A class that maintains version information about cars and tracks
class AUTOUPDATE
{
public:
	/// write the version information to a VDrift config format file
	/// returns true on success
	bool Write(const std::string & path) const;
	
	/// read the version information from a VDrift config format file
	/// returns true on success
	bool Load(const std::string & path);
	
	/// update folder/version pairs for the specified group in the available_updates map
	void SetAvailableUpdates(const std::string & group, const std::map <std::string, int> & new_revs);
	
	/// after SetUpdatesAvailable has been called,
	/// return a pair where the first element is the folders that are newer that what we have,
	/// and the second is the folders that we have that do not exist in the source
	std::pair <std::vector <std::string>,std::vector <std::string> > CheckUpdate(const std::string & group) const;
	
	/// return the current on disk (first) and latest available (second) revs for the item in the group
	std::pair <int, int> GetVersions(const std::string & group, const std::string & item) const;
	
	/// return available updates of the specified group
	std::map <std::string, int> GetAvailableUpdates(const std::string & group) const;
	
	/// returns true if we have no update data
	bool empty() const {return groups.empty();}
	bool empty(const std::string & group) const;
	
private:
	/// map from group name to folder/version pairs
	typedef std::map <std::string, int> pair_type;
	typedef std::map <std::string, pair_type> group_type;
	group_type groups;
	group_type available_updates;
};

#endif
