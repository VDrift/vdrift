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

#include "updatemanager.h"
#include "svn_sourceforge.h"
#include "pathmanager.h"
#include "gui/gui.h"
#include "http.h"
#include "utils.h"

#define REPO SvnSourceForge

UpdateManager::UpdateManager(AutoUpdate &update, std::ostream & info, std::ostream & err) :
	autoupdate(update),
	info_output(info),
	error_output(err),
	updates_num(0),
	cur_object_id(0)
{
	// Constructor
}

bool UpdateManager::Init(
	const std::string & updatefilebase,
	const std::string & newupdatefile,
	const std::string & newupdatefilebackup,
	const std::string & newguipage,
	const std::string & newgroup)
{
	updates_num = 0;
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

void UpdateManager::StartCheckForUpdates(GameDownloader downloader, Gui & gui)
{
	const bool verbose = true;
	gui.ActivatePage("Downloading", 0.25, error_output);

	// download svn folder view to a temporary folder
	const std::string url = autoupdate.GetMetaUrl() + group + "/";
	if (!downloader(url))
	{
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return;
	}

	// get the downloaded page
	info_output << "Checking for updates..." << std::endl;
	std::ifstream page(downloader.GetHttp().GetDownloadPath(url).c_str());
	if (!page)
	{
		error_output << "File not found: " << downloader.GetHttp().GetDownloadPath(url) << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return;
	}
	const std::map <std::string, int> res = REPO::ParseFolderView(page);

	// check the current SVN picture against what's in our update manager to find things we can update
	autoupdate.SetAvailableUpdates(group, res);
	std::pair <std::vector <std::string>, std::vector <std::string> > updates = autoupdate.CheckUpdate(group);
	info_output << "Updates: " << updates.first.size() << " update(s), " << updates.second.size() << " deletion(s) found" << std::endl;
	if (verbose)
	{
		info_output << "Updates: [";
		Utils::print_vector(updates.first, info_output);
		info_output << "]" << std::endl;
		info_output << "Deletions: [";
		Utils::print_vector(updates.second, info_output);
		info_output << "]" << std::endl;
	}

	updates_num = updates.first.size();

	// store the new set of available updates
	autoupdate.Write(updatefile);

	// attempt to download updates.config
	DownloadRemoteConfig(downloader);
}

void UpdateManager::Show(Gui & gui)
{
	const bool verbose = false;

	// build a list of current cars/tracks
	// including those that we don't have yet but know about
	std::set <std::string> allobjects;

	// start off with the list we have on disk
	for (std::vector<std::pair <std::string, std::string> >::const_iterator i = disklist.begin(); i != disklist.end(); i++)
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
	while (cur_object_id < 0)
		cur_object_id = allobjects.size() + cur_object_id;
	cur_object_id = cur_object_id % allobjects.size();

	std::set <std::string>::const_iterator it = allobjects.begin();
	std::advance(it, cur_object_id);
	objectname = *it;

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
		gui.SetLabelText(guipage, "version_local", Utils::tostr(revs.first));
		gui.SetLabelText(guipage, "version_remote", Utils::tostr(revs.second));

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

bool UpdateManager::ApplyUpdate(GameDownloader downloader, Gui & gui, const PathManager & pathmanager)
{
	if (objectname.empty())
	{
		error_output << "ApplyUpdate: No object set for update" << std::endl;
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
	assert(remoteconfig.size());
	int version = 0;
	if (!remoteconfig.get("formats", group, version, error_output))
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

	// make sure group dir exists
	PathManager::MakeDir(pathmanager.GetWriteableDataPath() + "/" + group + "/");

	// download object
	const std::string folder_url = autoupdate.GetFileUrl() + group + "/" + objectname + "/";
	const std::string folder_path = pathmanager.GetWriteableDataPath() + "/" + group + "/" + objectname + "/";
	if (!REPO::DownloadFolder(folder_url, folder_path, downloader, error_output))
	{
		error_output << "ApplyUpdate: download failed" << std::endl;
		gui.ActivatePage("DataConnectionError", 0.25, error_output);
		return false;
	}
	info_output << "ApplyUpdate: download successful: " << folder_path << std::endl;

	// record update
	if (!autoupdate.empty())
		autoupdate.Write(updatefilebackup); // save a backup before changing it
	autoupdate.SetVersion(group, objectname, revs.second);
	autoupdate.Write(updatefile);

	gui.ActivatePage("UpdateSuccessful", 0.25, error_output);
	return true;
}

bool UpdateManager::DownloadRemoteConfig(GameDownloader downloader)
{
	if (remoteconfig.size())
	{
		info_output << "DownloadRemoteConfig: already have the remote updates.config" << std::endl;
		return true;
	}

	const std::string url = autoupdate.GetFileUrl() + "settings/updates.config";
	info_output << "DownloadRemoteConfig: attempting to download " + url << std::endl;
	if (!downloader(url))
	{
		error_output << "DownloadRemoteConfig: download failed" << std::endl;
		return false;
	}

	const std::string filepath = downloader.GetHttp().GetDownloadPath(url);
	info_output << "DownloadRemoteConfig: download successful: " << filepath << std::endl;

	const std::string updatesconfig = Utils::LoadFileIntoString(filepath, error_output);
	if (updatesconfig.empty())
	{
		error_output << "DownloadRemoteConfig: empty updates.config" << std::endl;
		return false;
	}

	std::istringstream f(updatesconfig);
	remoteconfig.clear();
	if (!remoteconfig.load(f))
	{
		error_output << "DownloadRemoteConfig: failed to load updates.config" << std::endl;
		return false;
	}

	PathManager::RemoveFile(filepath);

	return true;
}
