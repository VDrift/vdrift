#include "pathmanager.h"
#include "definitions.h"

#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib> //getenv

//includes for listing folder contents
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

static void MakeDir(const std::string & dir)
{
#ifndef _WIN32
	mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	mkdir(dir.c_str());
#endif
}

void PATHMANAGER::Init(std::ostream & info_output, std::ostream & error_output)
{
	//figure out the user's home directory
	const char* homedir;
	#ifndef _WIN32
	homedir = getenv ( "HOME" );
	if ( homedir == NULL )
	{
		homedir = getenv ( "USER" );
		if ( homedir == NULL )
		{
			homedir = getenv ( "USERNAME" );
			if ( homedir == NULL )
			{
				error_output << "Could not find user's home directory!" << std::endl;
			}
		}
		home_directory = "/home/";
	}
	#else
	homedir = getenv ( "USERPROFILE" );
	if (homedir == NULL)
	{
		homedir = "data"; // WIN 9x/Me
	}
	#endif
	home_directory += homedir;

	//find data dir
	const char * datadir = getenv ( "VDRIFT_DATA_DIRECTORY" );
	if (datadir == NULL) {
		#ifndef _WIN32
		if (FileExists("data/settings/options.config")) {
			data_directory = "data";
		} else {
			data_directory = DATA_DIR;
		}
		#else
		data_directory = "D:/vdrift/data";//"data";
		#endif
	} else {
		data_directory = std::string(datadir);
	}

	//find settings file
	settings_path = home_directory;
	#ifndef _WIN32
	settings_path += "/";
	settings_path += SETTINGS_DIR;
	MakeDir(settings_path);
	#else
	#ifdef _WIN32
	MakeDir(settings_path+"\\My Documents");
	MakeDir(settings_path+"\\My Documents\\My Games");
	settings_path += "\\My Documents\\My Games\\VDrift";
	MakeDir(settings_path);
	#else
	{
		settings_path += "/";
		settings_path += SETTINGS_DIR;
	}
	#endif
	#endif

	temporary_folder = settings_path+"/tmp";

	MakeDir(GetTrackRecordsPath());
	MakeDir(GetReplayPath());
	MakeDir(GetScreenshotPath());
	MakeDir(GetTemporaryFolder());

	//print diagnostic info
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

bool PATHMANAGER::GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension) const
{
//------Folder listing code for POSIX
#ifndef _WIN32
	DIR *dp;
	struct dirent *ep;
	dp = opendir (folderpath.c_str());
	if (dp != NULL)
	{
		while ( ( ep = readdir( dp ) ) )
		{
			std::string newname = ep->d_name;
			if (newname[0] != '.')
			{
				outputfolderlist.push_back(newname);
			}
		}
		(void) closedir (dp);
	}
	else
	{
		return false;
	}
#else 	//------End POSIX-specific folder listing code ---- Start WIN32 Specific code
	HANDLE          hList;
	TCHAR           szDir[MAX_PATH+1];
	WIN32_FIND_DATA FileData;

	// Get the proper directory path
	sprintf(szDir, "%s\\*", folderpath.c_str ());

	// Get the first file
	hList = FindFirstFile(szDir, &FileData);
	if (hList != INVALID_HANDLE_VALUE)
	{
		// Traverse through the directory structure
		while (FindNextFile(hList, &FileData))
		{
			if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && 
				(FileData.cFileName[0] != '.'))
			{
				outputfolderlist.push_back (FileData.cFileName);
			}
		}
	}

	FindClose(hList);
#endif //------End WIN32 specific folder listing code

	//remove non-matcthing extensions
	if (!extension.empty())
	{
		std::list <std::list <std::string>::iterator> todel;
		for (std::list <std::string>::iterator i = outputfolderlist.begin(); i != outputfolderlist.end(); ++i)
		{
			if (i->find(extension) != i->length()-extension.length()) todel.push_back(i);
		}

		for (std::list <std::list <std::string>::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		{
			outputfolderlist.erase(*i);
		}
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

///only call this before Init()
void PATHMANAGER::SetProfile(const std::string & value)
{
	assert(data_directory.empty()); //assert that Init() hasn't been called yet
	profile_suffix = "."+value;
}
