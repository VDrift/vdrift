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

#ifndef _DOWNLOADABLE_H
#define _DOWNLOADABLE_H

#include <string>
#include <map>
#include <vector>

/// A dictionary of downloadable assets.
class DownloadableManager
{
public:
	/// Initialize using the provided filename as the record-keeping method.
	/// The file doesn't need to exist, but if it does, it will be parsed to get local_version info.
	void Initialize(const std::string & newfilename);

	/// Given a map of available downloadables and their remote version, return a list of downloadable names that we want to download.
	std::vector <std::string> GetUpdatables(const std::map <std::string, int> & remote_downloadables) const;

	/// Inform us that we have just installed a new downloadable.
	void SetDownloadable(const std::string & name, int new_version);

private:
	std::string filename;
    /// Mapping between downloadable name and local version.
	std::map <std::string, int> downloadables;

	void Load();
	void Save() const;
};

#endif
