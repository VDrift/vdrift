#ifndef _UPDATEMANAGEMENT_H
#define _UPDATEMANAGEMENT_H

#include "autoupdate.h"
#include "game_downloader.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>

class GUI;
class PATHMANAGER;
class CONFIG;

class UPDATE_MANAGER
{
public:
	UPDATE_MANAGER(AUTOUPDATE & autoupdate, std::ostream & info, std::ostream & err);

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

	void Show(GUI & gui);

	void StartCheckForUpdates(GAME_DOWNLOADER downloader, GUI & gui);

	// returns true on success
	bool ApplyUpdate(GAME_DOWNLOADER downloader, GUI & gui, const PATHMANAGER & pathmanager);

	std::map<std::string, std::list <std::pair <std::string, std::string> > > & GetValueLists() {return valuelists;}

private:
	AUTOUPDATE & autoupdate;
	std::ostream & info_output;
	std::ostream & error_output;

	int cur_object_id;
	std::string updatefilebase;
	std::string updatefile;
	std::string updatefilebackup;
	std::string guipage;
	std::string group;

	// mirrors mappings used in the GUI, used to get the list of cars/tracks on disk
	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;

	// holds settings/updates.config from HEAD in the SVN data repository
	std::auto_ptr <CONFIG> remoteconfig;

	// retrieve updates.config from HEAD in the SVN data repository and store it in remoteconfig
	// if remoteconfig is already set, returns true immediately
	// returns true on success
	bool DownloadRemoteConfig(GAME_DOWNLOADER downloader, GUI & gui);
};

#endif
