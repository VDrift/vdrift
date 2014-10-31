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

#ifndef _UPDATEMANAGER_H
#define _UPDATEMANAGER_H

#include "autoupdate.h"
#include "game_downloader.h"
#include "cfg/config.h"

#include <iosfwd>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>

class Gui;
class PathManager;
class Config;

class UpdateManager
{
public:
	UpdateManager(AutoUpdate & update, std::ostream & info, std::ostream & err);

	// returns true on success
	bool Init(
		const std::string & updatefilebase,
		const std::string & newupdatefile,
		const std::string & newupdatefilebackup,
		const std::string & guipage,
		const std::string & group);

	void Reset() {cur_object_id = 0;}

	void Increment() {cur_object_id++;}

	void Decrement() {cur_object_id--;}

	void Show(Gui & gui);

	void StartCheckForUpdates(GameDownloader downloader, Gui & gui);

	size_t GetUpdatesNum() const { return updates_num; }

	// returns true on success
	bool ApplyUpdate(GameDownloader downloader, Gui & gui, const PathManager & pathmanager);

	std::vector<std::pair<std::string, std::string> > & GetValueList() {return disklist;}

private:
	AutoUpdate & autoupdate;
	std::ostream & info_output;
	std::ostream & error_output;

	size_t updates_num;
	int cur_object_id;
	std::string objectname;
	std::string updatefilebase;
	std::string updatefile;
	std::string updatefilebackup;
	std::string guipage;
	std::string group;

	// mirrors mappings used in the GUI, used to get the list of cars/tracks on disk
	std::vector<std::pair<std::string, std::string> > disklist;

	// holds settings/updates.config from HEAD in the SVN data repository
	Config remoteconfig;

	// retrieve updates.config from HEAD in the SVN data repository and store it in remoteconfig
	// if remoteconfig is already set, returns true immediately
	// returns true on success
	bool DownloadRemoteConfig(GameDownloader downloader);
};

#endif
