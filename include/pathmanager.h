#ifndef _PATHMANAGER_H
#define _PATHMANAGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <list>

#include <cstdlib> //getenv

//includes for listing folder contents
#ifdef _WIN32
#undef NOMINMAX
#define NOMINMAX 1
#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#include <cassert>

class PATHMANAGER
{
	private:
		std::string home_directory;
		std::string settings_path;
		std::string data_directory;
		std::string profile_suffix;
		
		#ifndef _WIN32
		bool DirectoryExists(std::string filename) const
		{
			DIR *dp;
			dp = opendir(filename.c_str());
			if (dp != NULL) {
				closedir(dp);
				return true;
			} else {
				return false;
			}
		}
		#endif
		
		void MakeDir(const std::string & dir)
		{
#ifndef _WIN32
			mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
			mkdir(dir.c_str());
#endif
		}
	
	public:
		std::string GetDataPath() const {return data_directory;}
		std::string GetStartupFile() const {return settings_path+"/startingup.txt";}
		std::string GetTrackRecordsPath() const {return settings_path+"/records"+profile_suffix;}
		std::string GetSettingsFile() const {return settings_path+"/VDrift.config"+profile_suffix;}
		std::string GetLogFile() const {return settings_path+"/log.txt";}
		std::string GetTrackPath() const {return data_directory+"/tracks";}
		std::string GetCarPath() const {return data_directory+"/cars";}
		std::string GetCarSharedPath() const {return data_directory+"/carparts";}
		void Init(std::ostream & info_output, std::ostream & error_output);
		std::string GetFontPath(const std::string & skinname) const {return data_directory+"/skins/"+skinname+"/fonts";}
		std::string GetGUIMenuPath(const std::string & skinname) const {return data_directory+"/skins/"+skinname+"/menus";}
		std::string GetGUITexturePath(const std::string & skinname) const {return data_directory+"/skins/"+skinname+"/textures";}
		bool GetFolderIndex(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension="") const; ///<optionally filter for the given extension
		std::string GetOptionsFile() const {return data_directory + "/settings/options.config";}
		std::string GetVideoModeFile() const {return data_directory + "/lists/videomodes";}
		std::string GetCarControlsFile() const {return settings_path+"/controls.config"+profile_suffix;}
		std::string GetDefaultCarControlsFile() const {return data_directory+"/settings/controls.config";}
		std::string GetGenericSoundPath() const {return data_directory + "/sounds";}
		std::string GetHUDTexturePath() const {return data_directory + "/textures/hud";}
		std::string GetEffectsTexturePath() const {return data_directory + "/textures/effects";}
		std::string GetDriverPath() const {return data_directory+"/drivers";}
		std::string GetReplayPath() const {return settings_path+"/replays";}
		std::string GetScreenshotPath() const {return settings_path+"/screenshots";}
		std::string GetTireSmokeTexturePath() const {return data_directory + "/textures/smoke";}
		std::string GetStaticReflectionMap() const {return data_directory + "/textures/weather/cubereflection-nosun.png";}
		std::string GetStaticAmbientMap() const {return data_directory + "/textures/weather/cubelighting.png";}
		std::string GetShaderPath() const {return data_directory + "/shaders";}
		
		bool FileExists(const std::string & filename) const
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
		void SetProfile ( const std::string& value )
		{
			assert(data_directory.empty()); //assert that Init() hasn't been called yet
			profile_suffix = "."+value;
		}
	
};

#endif
