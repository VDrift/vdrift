#include "updatemanagement.h"

#include "svn_sourceforge.h"
#include "gui.h"
#include "http.h"
#include "pathmanager.h"
#include "utils.h"
#include "containeralgorithm.h"
#include "archiveutils.h"

typedef SVN_SOURCEFORGE repo_type;

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
	repo_type svn;
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
		
		// attempt to download updates.config
		DownloadRemoteConfig(downloader, gui);
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
		gui.SetLabelText("CarManager", "carlabel",  thecar);
		gui.SetLabelText("CarManager", "version", "Version: " + UTILS::tostr(revs.first));
		
		if (!revs.second)
		{
			// either they haven't checked for updates or the car is local only
			if (autoupdate.empty())
			{
				gui.SetLabelText("CarManager", "updateinfo", "You need to check for updates");
			}
			else
			{
				// the car doesn't exist remotely; either it was deleted from the remote repo or
				// the user created this car locally
				gui.SetLabelText("CarManager", "updateinfo", "This car cannot be updated");
			}
			gui.SetButtonEnabled("CarManager", "Updatebutton", false);
		}
		else
		{
			if (revs.first >= revs.second) // this checks that the local rev is at least the remote rev
			{
				gui.SetLabelText("CarManager", "updateinfo", "This car is up to date");
				gui.SetButtonEnabled("CarManager", "Updatebutton", false);
			}
			else // local rev is less than the remote rev
			{
				gui.SetLabelText("CarManager", "updateinfo", "An update is available");
				gui.SetButtonEnabled("CarManager", "Updatebutton", true);
				info_output << thecar << ": local version: " << revs.first << " remote version: " << revs.second << std::endl;
			}
		}
	}
}

bool UPDATE_MANAGER::ApplyCarUpdate(GAME_DOWNLOADER downloader, GUI & gui, const PATHMANAGER & pathmanager)
{
	const bool debug = false;
	const std::string group = "cars";
	std::string carname;
	if (!gui.GetLabelText("CarManager", "carlabel", carname))
	{
		error_output << "Couldn't find the name of the car to update" << std::endl;
		return false;
	}
	
	std::pair <int, int> revs = autoupdate.GetVersions(group, carname);
	
	if (revs.first == revs.second)
	{
		error_output << "ApplyCarUpdate: " << carname << " is already up to date" << std::endl;
		return false;
	}
	
	// check that we have a recent enough release
	if (!DownloadRemoteConfig(downloader, gui))
	{
		error_output << "ApplyCarUpdate: unable to retrieve version information" << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return false;
	}
	assert(remoteconfig.get()); // DownloadRemoteConfig should only return true if remoteconfig is set
	int version = 0;
	if (!remoteconfig->GetParam("formats", group, version, error_output))
	{
		error_output << "ApplyCarUpdate: missing version information" << std::endl;
		return false;
	}
	int localversion = autoupdate.GetFormatVersion(group);
	if (localversion != version)
	{
		error_output << "ApplyCarUpdate: remote format for " << group << " is " << version << " but this version of VDrift only understands " << localversion << std::endl;
		gui.ActivatePage("UpdateFailedVersion", 0.25, error_output);
		return false;
	}
	
	// download archive
	std::string url = repo_type::GetCarDownloadLink(carname);
	std::string archivepath = downloader.GetHttp().GetDownloadPath(url);
	if (debug && !pathmanager.FileExists(archivepath)) // if in debug mode, don't redownload files
	{
		info_output << "ApplyCarUpdate: attempting to download " + url << std::endl;
		bool success = downloader(url);
		if (!success)
		{
			error_output << "ApplyCarUpdate: download failed" << std::endl;
			gui.ActivatePage("DataConnectionError", 0.25, error_output);
			return false;
		}
	}
	info_output << "ApplyCarUpdate: download successful: " << archivepath << std::endl;
	
	// decompress and write output
	if (!Decompress(archivepath, pathmanager.GetWriteableCarPath(), info_output, error_output))
	{
		error_output << "ApplyCarUpdate: unable to decompress update" << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return false;
	}
	
	// record update
	if (!autoupdate.empty())
		autoupdate.Write(updatefilebackup); // save a backup before changing it
	autoupdate.SetVersion(group, carname, revs.second);
	
	// remove temporary file
	if (!debug)
	{
		PATHMANAGER::DeleteFile(archivepath);
	}
	
	gui.ActivatePage("UpdateSuccessful", 0.25, error_output);
	return true;
}

bool UPDATE_MANAGER::DownloadRemoteConfig(GAME_DOWNLOADER downloader, GUI & gui)
{
	if (remoteconfig.get())
	{
		info_output << "DownloadRemoteConfig: already have the remote updates.config" << std::endl;
		return true;
	}
	
	std::string url = repo_type::GetRemoteUpdateConfigUrl();
	info_output << "DownloadRemoteConfig: attempting to download " + url << std::endl;
	bool success = downloader(url);
	if (!success)
	{
		error_output << "DownloadRemoteConfig: download failed" << std::endl;
		return false;
	}
	
	std::string filepath = downloader.GetHttp().GetDownloadPath(url);
	info_output << "DownloadRemoteConfig: download successful: " << filepath << std::endl;
	
	std::string updatesconfig = UTILS::LoadFileIntoString(filepath, error_output);
	if (updatesconfig.empty())
	{
		error_output << "DownloadRemoteConfig: empty updates.config" << std::endl;
		return false;
	}
	
	std::stringstream f(updatesconfig);
	CONFIG * conf = new CONFIG();
	remoteconfig.reset(conf);
	if (!conf->Load(f))
	{
		error_output << "DownloadRemoteConfig: failed to load updates.config" << std::endl;
		return false;
	}
	
	PATHMANAGER::DeleteFile(filepath);
	
	return true;
}
