#include "updatemanager.h"

#include "svn_sourceforge.h"
#include "gui/gui.h"
#include "http.h"
#include "pathmanager.h"
#include "utils.h"
#include "containeralgorithm.h"
#include "archiveutils.h"

typedef SVN_SOURCEFORGE repo_type;

UPDATE_MANAGER::UPDATE_MANAGER(AUTOUPDATE & autoupdate, std::ostream & info, std::ostream & err) :
	autoupdate(autoupdate), info_output(info), error_output(err), cur_object_id(0)
{
	// Constructor
}

bool UPDATE_MANAGER::Init(
	const std::string & updatefilebase,
	const std::string & newupdatefile,
	const std::string & newupdatefilebackup,
	const std::string & newguipage,
	const std::string & newgroup)
{
	updatefile = newupdatefile;
	updatefilebackup = newupdatefilebackup;
	guipage = newguipage;
	group = newgroup;

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
	repo_type svn;
	std::string url = autoupdate.GetUrl() + group + "/";
	if (!downloader(url))
	{
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return;
	}

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

	if (!updates.first.empty())
	{
		std::stringstream updatesummary;
		updatesummary << updates.first.size() << " " << group << " update";
		if (updates.first.size() > 1)
			updatesummary << "s";
		updatesummary << " available";
		gui.SetLabelText("UpdatesFound", group, updatesummary.str());
	}

	// store the new set of available updates
	autoupdate.Write(updatefile);

	// attempt to download updates.config
	DownloadRemoteConfig(downloader);
}

void UPDATE_MANAGER::Show(GUI & gui)
{
	const bool verbose = false;

	// build a list of current cars/tracks
	// including those that we don't have yet but know about
	std::set <std::string> allobjects;

	// start off with the list of cars/tracks we have on disk
	const std::list <std::pair <std::string, std::string> > & disklist = valuelists[group];
	for (std::list <std::pair <std::string, std::string> >::const_iterator i = disklist.begin(); i != disklist.end(); i++)
	{
		allobjects.insert(i->first);
	}

	// now add in the cars/tracks in the remote repo
	std::map <std::string, int> remote = autoupdate.GetAvailableUpdates(group);
	for (std::map <std::string, int>::const_iterator i = remote.begin(); i != remote.end(); i++)
	{
		allobjects.insert(i->first);
	}

	if (allobjects.empty())
	{
		error_output << "Show: No " + group + "!" << std::endl;
		return;
	}

	// find the car/track at index cur_object_id
	std::string objectname;
	int count = 0;
	while (cur_object_id < 0)
		cur_object_id = allobjects.size() + cur_object_id;
	cur_object_id = cur_object_id % allobjects.size();
	for (std::set <std::string>::const_iterator i = allobjects.begin(); i != allobjects.end(); i++, count++)
	{
		if (count == cur_object_id)
		{
			objectname = *i;
		}
	}

	if (verbose)
	{
		info_output << "All " + group + ": ";
		for (std::set <std::string>::const_iterator i = allobjects.begin(); i != allobjects.end(); i++)
		{
			if (i != allobjects.begin())
				info_output << ", ";
			info_output << *i;
		}
		info_output << std::endl;
	}

	if (!objectname.empty())
	{
		std::pair <int, int> revs = autoupdate.GetVersions(group, objectname);
		gui.ActivatePage(guipage, 0.0001, error_output);
		gui.SetLabelText(guipage, "name", objectname);
		gui.SetLabelText(guipage, "version_local", UTILS::tostr(revs.first));
		gui.SetLabelText(guipage, "version_remote", UTILS::tostr(revs.second));

		if (!revs.second)
		{
			// either they haven't checked for updates or the car/track is local only
			if (autoupdate.empty())
			{
				gui.SetLabelText(guipage, "update_info", "Check for updates");
			}
			else
			{
				// car/track doesn't exist remotely; either it was deleted from the remote repo or
				// the user created this car/track locally
				gui.SetLabelText(guipage, "update_info", "No update available");
			}
		}
		else
		{
			if (revs.first >= revs.second) // this checks that the local rev is at least the remote rev
			{
				gui.SetLabelText(guipage, "update_info", "Local version up to date");
			}
			else // local rev is less than the remote rev
			{
				gui.SetLabelText(guipage, "update_info", "An update is available");
				info_output << objectname << ": local version: " << revs.first << " remote version: " << revs.second << std::endl;
			}
		}
	}
}

bool UPDATE_MANAGER::ApplyUpdate(GAME_DOWNLOADER downloader, GUI & gui, const PATHMANAGER & pathmanager)
{
	const bool debug = false;
	std::string objectname;
	if (!gui.GetLabelText(guipage, "name", objectname))
	{
		error_output << "Couldn't find the name label to update in " + guipage + "." << std::endl;
		return false;
	}

	std::pair <int, int> revs = autoupdate.GetVersions(group, objectname);
	if (revs.first == revs.second)
	{
		error_output << "ApplyUpdate: " << objectname << " is already up to date" << std::endl;
		return false;
	}

	// check that we have a recent enough release
	if (!DownloadRemoteConfig(downloader))
	{
		error_output << "ApplyUpdate: unable to retrieve version information" << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return false;
	}

	// DownloadRemoteConfig should only return true if remoteconfig is set
	assert(remoteconfig.get());
	int version = 0;
	if (!remoteconfig->GetParam("formats", group, version, error_output))
	{
		error_output << "ApplyUpdate: missing version information" << std::endl;
		return false;
	}

	int localversion = autoupdate.GetFormatVersion(group);
	if (localversion != version)
	{
		error_output << "ApplyUpdate: remote format for " << group << " is " << version << " but this version of VDrift only understands " << localversion << std::endl;
		gui.ActivatePage("UpdateFailedVersion", 0.25, error_output);
		return false;
	}

	// download archive
	std::string url = repo_type::GetDownloadLink(autoupdate.GetUrl(), group, objectname);
	std::string archivepath = downloader.GetHttp().GetDownloadPath(url);
	if (!(debug && pathmanager.FileExists(archivepath))) // if in debug mode, don't redownload files
	{
		info_output << "ApplyUpdate: attempting to download " + url << std::endl;
		bool success = downloader(url);
		if (!success)
		{
			error_output << "ApplyUpdate: download failed" << std::endl;
			gui.ActivatePage("DataConnectionError", 0.25, error_output);
			return false;
		}
	}
	info_output << "ApplyUpdate: download successful: " << archivepath << std::endl;

	// decompress and write output
	if (!Decompress(archivepath, pathmanager.GetWriteableDataPath()+"/"+group, info_output, error_output))
	{
		error_output << "ApplyUpdate: unable to decompress update" << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return false;
	}

	// record update
	if (!autoupdate.empty())
		autoupdate.Write(updatefilebackup); // save a backup before changing it
	autoupdate.SetVersion(group, objectname, revs.second);
	autoupdate.Write(updatefile);

	// remove temporary file
	if (!debug)
	{
		PATHMANAGER::DeleteFile1(archivepath);
	}

	gui.ActivatePage("UpdateSuccessful", 0.25, error_output);
	return true;
}

bool UPDATE_MANAGER::DownloadRemoteConfig(GAME_DOWNLOADER downloader)
{
	if (remoteconfig.get())
	{
		info_output << "DownloadRemoteConfig: already have the remote updates.config" << std::endl;
		return true;
	}

	std::string url = autoupdate.GetUrl() + "settings/updates.config";
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

	PATHMANAGER::DeleteFile1(filepath);

	return true;
}
