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
	UPDATE_MANAGER(std::ostream & info, std::ostream & err) : info_output(info), error_output(err), car_manager_cur_car(0) {}
	
	// returns true on success
	bool Init(const std::string & updatefilebase, const std::string & newupdatefile, const std::string & newupdatefilebackup);
	
	void ResetCarManager() {car_manager_cur_car = 0;}
	void IncrementCarManager() {car_manager_cur_car++;}
	void DecrementCarManager() {car_manager_cur_car--;}
	void ShowCarManager(GUI & gui);
	
	void StartCheckForUpdates(GAME_DOWNLOADER downloader, GUI & gui);
	
	// returns true on success
	bool ApplyCarUpdate(GAME_DOWNLOADER downloader, GUI & gui, const PATHMANAGER & pathmanager);
	
	std::map<std::string, std::list <std::pair <std::string, std::string> > > & GetValueLists() {return valuelists;}
	
private:
	std::ostream & info_output;
	std::ostream & error_output;
	
	AUTOUPDATE autoupdate;
	int car_manager_cur_car;
	
	std::string updatefilebase;
	std::string updatefile;
	std::string updatefilebackup;
	
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
