#ifndef _DOWNLOADABLE_H
#define _DOWNLOADABLE_H

#include <string>
#include <map>
#include <vector>

/// A dictionary of downloadable assets
class DOWNLOADABLEMANAGER
{
public:
	/// initialize using the provided filename as the record-keeping method
	/// the file doesn't need to exist, but if it does, it will be parsed
	/// to get local_version info
	void Initialize(const std::string & newfilename);
	
	/// given a map of available downloadables and their remote version,
	/// return a list of downloadable names that we want to download
	std::vector <std::string> GetUpdatables(const std::map <std::string, int> & remote_downloadables) const;
	
	/// inform us that we have just installed a new downloadable
	void SetDownloadable(const std::string & name, int new_version);
	
private:
	std::string filename;
	std::map <std::string, int> downloadables; ///< mapping between downloadable name and local version
	
	void Load();
	void Save() const;
};

#endif
