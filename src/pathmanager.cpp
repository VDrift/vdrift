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

#include "definitions.h"
#include "pathmanager.h"

#include <fstream>
#include <cassert>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h> // rmdir
#include <errno.h>
#endif

// true if ext empty or matches end of str
static bool has_extension(const std::string & str, const std::string & ext)
{
	int strn = str.length();
	int extn = ext.length();
	if (extn >= strn)
		return false;
	while (extn > 0)
		if (ext[--extn] != str[--strn])
			return false;
	return true;
}

void PathManager::Init(std::ostream & info_output, std::ostream & error_output)
{
	// Figure out the user's home directory.
	const char * homedir;
#ifndef _WIN32
	if ((homedir = getenv("HOME")) == NULL)
	{
		if ((homedir = getenv("USER")) == NULL)
			if ((homedir = getenv("USERNAME")) == NULL)
				error_output << "Could not find user's home directory!" << std::endl;
		home_directory = "/home/";
	}
#else
	if ((homedir = getenv("USERPROFILE")) == NULL)
		homedir = "data";	// WIN 9x/Me
#endif
	home_directory += homedir;

	// Set data dir.
	const char * datadir = getenv("VDRIFT_DATA_DIRECTORY");
	if (datadir == NULL)
#if !defined(_WIN32) && !defined(__APPLE__)
		if (FileExists("data/settings/options.config"))
			data_directory = "data";
		else
			data_directory = DATA_DIR;
#elif __APPLE__
		data_directory = get_mac_data_dir();
#else
		data_directory = "data";
#endif
	else
		data_directory = std::string(datadir);

	// Set settings dir.
#ifdef _WIN32
	const char * appdata = getenv("APPDATA");
	if (appdata)
		settings_path = std::string(appdata) + "\\VDrift";
	else
		settings_path = home_directory + "\\VDrift";
#else
	settings_path = home_directory + "/" + SETTINGS_DIR;
#endif

	temporary_folder = settings_path + "/tmp";

	MakeDir(settings_path);
	MakeDir(GetTrackRecordsPath());
	MakeDir(GetReplayPath());
	MakeDir(GetScreenshotPath());
	MakeDir(GetTemporaryFolder());

	// Print diagnostic info.
	info_output << "Home directory: " << home_directory << std::endl;
	info_output << "Settings file: " << GetSettingsFile();
	if (!FileExists(GetSettingsFile()))
		info_output << " (does not exist, will be created)";
	info_output << std::endl;
	info_output << "Data directory: " << data_directory;
	if (datadir)
		info_output << "\nVDRIFT_DATA_DIRECTORY: " << datadir;
#if !defined(_WIN32) && !defined(__APPLE__)
	info_output << "\nDATA_DIR: " << DATA_DIR;
#endif
	info_output << std::endl;
	info_output << "Temporary directory: " << GetTemporaryFolder() << std::endl;
	info_output << "Log file: " << GetLogFile() << std::endl;
}

void PathManager::SetProfile(const std::string & value)
{
	// Assert that Init() hasn't been called yet.
	assert(data_directory.empty());
	profile_suffix = "." + value;
}

bool PathManager::GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension) const
{
	// Folder listing code for POSIX.
#ifndef _WIN32
	DIR *dp;
	struct dirent *ep;
	dp = opendir(folderpath.c_str());
	if (dp != NULL)
	{
		while ((ep = readdir(dp)))
		{
			std::string newname = ep->d_name;
			if (newname[0] != '.' && has_extension(newname, extension))
				outputfolderlist.push_back(newname);
		}
		closedir(dp);
	}
	else
	{
		return false;
	}
	// End POSIX-specific folder listing code. Start WIN32 Specific code.
#else
	HANDLE          hList;
	TCHAR           szDir[MAX_PATH+1];
	WIN32_FIND_DATA FileData;

	// Get the proper directory path.
	sprintf(szDir, "%s\\*", folderpath.c_str());

	// Get the first file.
	hList = FindFirstFile(szDir, &FileData);
	if (hList != INVALID_HANDLE_VALUE)
	{
		// Traverse through the directory structure.
		while (FindNextFile(hList, &FileData))
		{
			std::string newname = FileData.cFileName;
			if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
				(newname[0] != '.') && has_extension(newname, extension))
				outputfolderlist.push_back(newname);
		}
		FindClose(hList);
	}
	else
	{
		return false;
	}
	// End WIN32 specific folder listing code.
#endif

	outputfolderlist.sort();
	return true;
}

bool PathManager::FileExists(const std::string & filename) const
{
	std::ifstream test;
	test.open(filename.c_str());
	if (test)
	{
		test.close();
		return true;
	}
	else
		return false;
}

void PathManager::CopyFileTo(const std::string & oldname, const std::string & newname)
{
	std::ifstream fi(oldname.c_str(), std::ios::binary);
	std::ofstream fo(newname.c_str(), std::ios::binary);
	fo << fi.rdbuf();
}

void PathManager::MakeDir(const std::string & dir)
{
#ifndef _WIN32
	mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	mkdir(dir.c_str());
#endif
}

void PathManager::RemoveDir(const std::string & dir)
{
#ifndef _WIN32
	rmdir(dir.c_str());
#else
	_rmdir(dir.c_str());
#endif
}

void PathManager::RemoveFile(const std::string & path)
{
	remove(path.c_str());
}

std::string PathManager::GetDataPath() const
{
	return data_directory;
}

std::string PathManager::GetWriteableDataPath() const
{
	return settings_path;
}

std::string PathManager::GetCarPartsPath() const
{
	return GetDataPath()+"/carparts";
}

std::string PathManager::GetTrackPartsPath() const
{
	return GetDataPath()+"/trackparts";
}

std::string PathManager::GetStartupFile() const
{
	return settings_path+"/startingup.txt";
}

std::string PathManager::GetTrackRecordsPath() const
{
	return settings_path+"/records"+profile_suffix;
}

std::string PathManager::GetSettingsFile() const
{
	return settings_path+"/VDrift.config"+profile_suffix;
}

std::string PathManager::GetLogFile() const
{
	return settings_path+"/log.txt";
}

std::string PathManager::GetTracksPath(const std::string & trackname) const
{
    // Check writeable track path first (check for presence of .txt files).
	if (FileExists(GetWriteableDataPath() + "/" + GetTracksDir() + "/" + trackname + "/" + "track.txt"))
		return GetWriteableDataPath() + "/" + GetTracksDir() + "/" + trackname;
	else
		return GetDataPath() + "/" + GetTracksDir()  + "/" + trackname;
}

std::string PathManager::GetCarPath(const std::string & carname) const
{
	// Check writeable car path first (check for presence of .car files).
	if (FileExists(GetWriteableDataPath() + "/" + GetCarsDir() + "/" + carname + "/" + carname + ".car"))
		return GetWriteableDataPath() + "/" + GetCarsDir() + "/" + carname;
	else
		return GetDataPath()+"/"+GetCarsDir()+"/"+carname;
}

std::string PathManager::GetCarPaintPath(const std::string & carname) const
{
	return GetCarPath(carname)+"/skins";
}

std::string PathManager::GetGUIMenuPath(const std::string & skinname) const
{
	return GetDataPath()+"/skins/"+skinname+"/menus";
}

std::string PathManager::GetSkinsPath() const
{
	return GetDataPath()+"/skins";
}

std::string PathManager::GetOptionsFile() const
{
	return GetDataPath() + "/settings/options.config";
}

std::string PathManager::GetCarControlsFile() const
{
	return settings_path+"/controls.config"+profile_suffix;
}

std::string PathManager::GetDefaultCarControlsFile() const
{
	return GetDataPath()+"/settings/controls.config";
}

std::string PathManager::GetReplayPath() const
{
	return settings_path+"/replays";
}

std::string PathManager::GetScreenshotPath() const
{
	return settings_path+"/screenshots";
}

std::string PathManager::GetStaticReflectionMap() const
{
	return GetDataPath()+"/textures/weather/cubereflection-nosun.png";
}

std::string PathManager::GetStaticAmbientMap() const
{
	return GetDataPath()+"/textures/weather/cubelighting.png";
}

std::string PathManager::GetShaderPath() const
{
	return GetDataPath() + "/shaders";
}

std::string PathManager::GetUpdateManagerFile() const
{
	return settings_path+"/updates.config"+profile_suffix;
}

std::string PathManager::GetUpdateManagerFileBackup() const
{
	return settings_path+"/updates.config.backup"+profile_suffix;
}

std::string PathManager::GetUpdateManagerFileBase() const
{
	return GetDataPath() + "/settings/updates.config";
}

std::string PathManager::GetTracksDir() const
{
	return "tracks";
}

std::string PathManager::GetCarsDir() const
{
	return "cars";
}

std::string PathManager::GetCarPartsDir() const
{
	return "carparts";
}

std::string PathManager::GetSkinsDir() const
{
	return "skins";
}

std::string PathManager::GetGUITextureDir(const std::string & skinname) const
{
	return GetSkinsDir()+"/"+skinname+"/textures";
}

std::string PathManager::GetGUILanguageDir(const std::string & skinname) const
{
	return GetSkinsDir()+"/"+skinname+"/languages";
}

std::string PathManager::GetFontDir(const std::string & skinname) const
{
	return GetSkinsDir()+"/"+skinname+"/fonts";
}

std::string PathManager::GetGenericSoundDir() const
{
	return "sounds";
}

std::string PathManager::GetHUDTextureDir() const
{
	return "textures/hud";
}

std::string PathManager::GetEffectsTextureDir() const
{
	return "textures/effects";
}

std::string PathManager::GetTireSmokeTextureDir() const
{
	return "textures/smoke";
}

std::string PathManager::GetReadOnlyCarsPath() const
{
	return GetDataPath()+"/"+GetCarsDir();
}

std::string PathManager::GetWriteableCarsPath() const
{
	return GetWriteableDataPath()+"/"+GetCarsDir();
}

std::string PathManager::GetReadOnlyTracksPath() const
{
	return GetDataPath()+"/"+GetTracksDir();
}

std::string PathManager::GetWriteableTracksPath() const
{
	return GetWriteableDataPath()+"/"+GetTracksDir();
}

std::string PathManager::GetTemporaryFolder() const
{
	return temporary_folder;
}
