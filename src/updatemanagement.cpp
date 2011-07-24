#include "updatemanagement.h"

#include "svn_sourceforge.h"
#include "gui.h"
#include "http.h"

bool UPDATE_MANAGER::Init(const std::string & updatefilebase, const std::string & newupdatefile, const std::string & newupdatefilebackup)
{
	updatefile = newupdatefile;
	updatefilebackup = newupdatefilebackup;
	
	// first try to load car/track versions from the tracking file in the user folder
	if (!autoupdate.Load(updatefile))
	{
		info_output << "Update status file " << updatefile << " will be created" << std::endl;
		
		// if that failed, use the version info that came with the release
		if (!autoupdate.Load(updatefilebase))
		{
			error_output << "Unable to load update manager base file: " << updatefilebase << "; updates will start from scratch" << std::endl;
			
			return false;
		}
	}
	
	return true;
}

void UPDATE_MANAGER::StartCheckForUpdates(GAME_DOWNLOADER downloader, GUI & gui)
{
	const bool verbose = true;
	
	gui.ActivatePage("Downloading", 0.25, error_output);

	// download the SVN folder views to a temporary folder
	std::string group = "cars";
	// TODO: track support
	SVN_SOURCEFORGE svn;
	std::string url = svn.GetCarFolderUrl();
	bool success = downloader(url);

	if (success)
	{
		// get the downloaded page
		info_output << "Checking for updates..." << std::endl;
		std::string page = UTILS::LoadFileIntoString(downloader.GetHttp().GetDownloadPath(url), error_output);
		std::map <std::string, int> res = svn.ParseFolderView(page);
		
		// check the current SVN picture against what's in our update manager to find things we can update
		autoupdate.SetAvailableUpdates(group, res);
		std::pair <std::vector <std::string>,std::vector <std::string> > updates = autoupdate.CheckUpdate(group);
		info_output << "Updates: " << updates.first.size() << " update(s), " << updates.second.size() << " deletion(s) found" << std::endl;
		if (verbose)
		{
			info_output << "Updates: [";
			UTILS::print_vector(updates.first, info_output);
			info_output << "]" << std::endl;
			info_output << "Deletions: [";
			UTILS::print_vector(updates.second, info_output);
			info_output << "]" << std::endl;
		}
		
		if (updates.first.empty())
		{
			gui.ActivatePage("UpdatesNotFound", 0.25, error_output);
		}
		else
		{
			gui.ActivatePage("UpdatesFound", 0.25, error_output);
			std::stringstream updatesummary;
			updatesummary << updates.first.size() << " update";
			if (updates.first.size() > 1)
				updatesummary << "s";
			updatesummary << " available";
			gui.SetLabelText("UpdatesFound", "LabelText", updatesummary.str());
		}
		
		// store the new set of available updates
		autoupdate.Write(updatefile);
	}
	else
	{
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
	}
}

void UPDATE_MANAGER::ShowCarManager(GUI & gui)
{
	const bool verbose = false;
	const std::string group = "cars";
	
	// build a list of current cars, including those that we don't have yet but know about
	std::set <std::string> allcars;
	
	// start off with the list of cars we have on disk
	const std::list <std::pair <std::string, std::string> > & cardisklist = valuelists["cars"];
	for (std::list <std::pair <std::string, std::string> >::const_iterator i = cardisklist.begin(); i != cardisklist.end(); i++)
	{
		allcars.insert(i->second);
	}
	
	// now add in the cars in the remote repo
	std::map <std::string, int> remote = autoupdate.GetAvailableUpdates(group);
	for (std::map <std::string, int>::const_iterator i = remote.begin(); i != remote.end(); i++)
	{
		allcars.insert(i->first);
	}
	
	if (allcars.empty())
	{
		error_output << "ShowCarManager: No cars!" << std::endl;
		return;
	}
	
	// find the car at index car_manager_cur_car
	std::string thecar;
	int count = 0;
	while (car_manager_cur_car < 0)
		car_manager_cur_car = allcars.size() + car_manager_cur_car;
	car_manager_cur_car = car_manager_cur_car % allcars.size();
	for (std::set <std::string>::const_iterator i = allcars.begin(); i != allcars.end(); i++, count++)
	{
		if (count == car_manager_cur_car)
		{
			thecar = *i;
		}
	}
	
	if (verbose)
	{
		info_output << "All cars: ";
		for (std::set <std::string>::const_iterator i = allcars.begin(); i != allcars.end(); i++)
		{
			if (i != allcars.begin())
				info_output << ", ";
			info_output << *i;
		}
		info_output << std::endl;
	}
	
	if (!thecar.empty())
	{
		std::pair <int, int> revs = autoupdate.GetVersions(group, thecar);
		gui.ActivatePage("CarManager", 0.0001, error_output);
		gui.SetLabelText("CarManager", "carlabel", thecar);
		gui.SetLabelText("CarManager", "localrev", UTILS::tostr(revs.first));
		gui.SetLabelText("CarManager", "remoterev", UTILS::tostr(revs.second));
	}
	
	/*if (!update_manager.empty())
		update_manager.Write(pathmanager.GetUpdateManagerFileBackup()); // save a backup before changing it*/
}

