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

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#endif
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include "pathmanager.h"
#include "definitions.h"

void PATHMANAGER::Init(std::ostream & info_output, std::ostream & error_output)
{
	// Figure out the user's home directory.
	const char* homedir;
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

	// Find data dir.
	const char * datadir = getenv("VDRIFT_DATA_DIRECTORY");
	if (datadir == NULL)
#ifndef _WIN32
		if (FileExists("data/settings/options.config"))
			data_directory = "data";
        else
			data_directory = DATA_DIR;
#else
		data_directory = "data";
#endif
        else
		data_directory = std::string(datadir);

	// Find settings file.
	settings_path = home_directory;
#ifdef _WIN32
	settings_path += "\\Documents\\VDrift";
	MakeDir(settings_path);
#else
	settings_path += "/";
	settings_path += SETTINGS_DIR;
	MakeDir(settings_path);
#endif

	temporary_folder = settings_path+"/tmp";

	MakeDir(GetTrackRecordsPath());
	MakeDir(GetReplayPath());
	MakeDir(GetScreenshotPath());
	MakeDir(GetTemporaryFolder());

	// Print diagnostic info.
	info_output << "Home directory: " << home_directory << std::endl;
	bool settings_file_present = FileExists(GetSettingsFile());
	info_output << "Settings file: " << GetSettingsFile();
	if (!settings_file_present)
		info_output << " (does not exist, will be created)";
	info_output << std::endl;
	info_output << "Data directory: " << data_directory;
	if (datadir)
		info_output << "\nVDRIFT_DATA_DIRECTORY: " << datadir;
#ifndef _WIN32
	info_output << "\nDATA_DIR: " << DATA_DIR;
#endif
	info_output << std::endl;
	info_output << "Temporary directory: " << GetTemporaryFolder() << std::endl;
	info_output << "Log file: " << GetLogFile() << std::endl;
}

void PATHMANAGER::SetProfile(const std::string & value)
{
	// Assert that Init() hasn't been called yet.
	assert(data_directory.empty());
	profile_suffix = "."+value;
}

bool PATHMANAGER::GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension) const
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
			if (newname[0] != '.')
				outputfolderlist.push_back(newname);
		}
		closedir(dp);
	}
	else
		return false;
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
		// Traverse through the directory structure.
		while (FindNextFile(hList, &FileData))
			if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && (FileData.cFileName[0] != '.'))
				outputfolderlist.push_back(FileData.cFileName);

	FindClose(hList);
	// End WIN32 specific folder listing code.
#endif

	// Remove non-matcthing extensions.
	if (!extension.empty())
	{
		std::list <std::list <std::string>::iterator> todel;
		for (std::list <std::string>::iterator i = outputfolderlist.begin(); i != outputfolderlist.end(); ++i)
			if (i->find(extension) != i->length()-extension.length())
                todel.push_back(i);

		for (std::list <std::list <std::string>::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
			outputfolderlist.erase(*i);
	}

	outputfolderlist.sort();
	return true;
}

bool PATHMANAGER::FileExists(const std::string & filename) const
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

void PATHMANAGER::MakeDir(const std::string & dir)
{
#ifndef _WIN32
	mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	mkdir(dir.c_str());
#endif
}

void PATHMANAGER::DeleteFile1(const std::string & path)
{
	remove(path.c_str());
}

std::string PATHMANAGER::GetDataPath() const
{
	return data_directory;
}

std::string PATHMANAGER::GetWriteableDataPath() const
{
	return settings_path;
}

std::string PATHMANAGER::GetCarPartsPath() const
{
	return GetDataPath()+"/"+GetCarPartsDir();
}

std::string PATHMANAGER::GetTrackPartsPath() const
{
	return GetDataPath()+"/trackparts";
}

std::string PATHMANAGER::GetStartupFile() const
{
	return settings_path+"/startingup.txt";
}

std::string PATHMANAGER::GetTrackRecordsPath() const
{
	return settings_path+"/records"+profile_suffix;
}

std::string PATHMANAGER::GetSettingsFile() const
{
	return settings_path+"/VDrift.config"+profile_suffix;
}

std::string PATHMANAGER::GetLogFile() const
{
	return settings_path+"/log.txt";
}

std::string PATHMANAGER::GetTracksPath() const
{
	return GetDataPath()+"/"+GetTracksDir();
}

std::string PATHMANAGER::GetCarPath(const std::string & carname) const
{
	// Check writeable car path first (check for presence of .car files).
	if (FileExists(GetWriteableDataPath() + "/" + GetCarsDir() + "/" + carname + "/" + carname + ".car"))
		return GetWriteableDataPath() + "/" + GetCarsDir() + "/" + carname;
	else
		return GetDataPath()+"/"+GetCarsDir()+"/"+carname;
}

std::string PATHMANAGER::GetCarPaintPath(const std::string & carname) const
{
	return GetCarPath(carname)+"/skins";
}

std::string PATHMANAGER::GetGUIMenuPath(const std::string & skinname) const
{
	return GetDataPath()+"/skins/"+skinname+"/menus";
}

std::string PATHMANAGER::GetSkinPath() const
{
	return GetDataPath()+"/skins/";
}

std::string PATHMANAGER::GetOptionsFile() const
{
	return GetDataPath() + "/settings/options.config";
}

std::string PATHMANAGER::GetVideoModeFile() const
{
	return GetDataPath() + "/lists/videomodes";
}

std::string PATHMANAGER::GetCarControlsFile() const
{
	return settings_path+"/controls.config"+profile_suffix;
}

std::string PATHMANAGER::GetDefaultCarControlsFile() const
{
	return GetDataPath()+"/settings/controls.config";
}

std::string PATHMANAGER::GetReplayPath() const
{
	return settings_path+"/replays";
}

std::string PATHMANAGER::GetScreenshotPath() const
{
	return settings_path+"/screenshots";
}

std::string PATHMANAGER::GetStaticReflectionMap() const
{
	return GetDataPath()+"/textures/weather/cubereflection-nosun.png";
}

std::string PATHMANAGER::GetStaticAmbientMap() const
{
	return GetDataPath()+"/textures/weather/cubelighting.png";
}

std::string PATHMANAGER::GetShaderPath() const
{
	return GetDataPath() + "/shaders";
}

std::string PATHMANAGER::GetUpdateManagerFile() const
{
	return settings_path+"/updates.config"+profile_suffix;
}

std::string PATHMANAGER::GetUpdateManagerFileBackup() const
{
	return settings_path+"/updates.config.backup"+profile_suffix;
}

std::string PATHMANAGER::GetUpdateManagerFileBase() const
{
	return GetDataPath() + "/settings/updates.config";
}

std::string PATHMANAGER::GetTracksDir() const
{
	return "tracks";
}

std::string PATHMANAGER::GetCarsDir() const
{
	return "cars";
}

std::string PATHMANAGER::GetCarPartsDir() const
{
    return "carparts";
}

std::string PATHMANAGER::GetGUITextureDir(const std::string & skinname) const
{
	return "skins/"+skinname+"/textures";
}

std::string PATHMANAGER::GetGUILanguageDir(const std::string & skinname) const
{
	return "skins/"+skinname+"/languages";
}

std::string PATHMANAGER::GetFontDir(const std::string & skinname) const
{
	return "/skins/"+skinname+"/fonts";
}

std::string PATHMANAGER::GetGenericSoundDir() const
{
	return "sounds";
}

std::string PATHMANAGER::GetHUDTextureDir() const
{
	return "textures/hud";
}

std::string PATHMANAGER::GetEffectsTextureDir() const
{
	return "textures/effects";
}

std::string PATHMANAGER::GetTireSmokeTextureDir() const
{
	return "textures/smoke";
}

std::string PATHMANAGER::GetReadOnlyCarsPath() const
{
	return GetDataPath()+"/"+GetCarsDir();
}

std::string PATHMANAGER::GetWriteableCarsPath() const
{
	return GetWriteableDataPath()+"/"+GetCarsDir();
}

std::string PATHMANAGER::GetReadOnlyTracksPath() const
{
	return GetDataPath()+"/"+GetTracksDir();
}

std::string PATHMANAGER::GetWriteableTracksPath() const
{
	return GetWriteableDataPath()+"/"+GetTracksDir();
}

std::string PATHMANAGER::GetTemporaryFolder() const
{
	return temporary_folder;
}
