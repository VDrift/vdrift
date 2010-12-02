#ifndef _PATHMANAGER_H
#define _PATHMANAGER_H

#include <list>
#include <string>

class PATHMANAGER
{
public:
	std::string GetDataPath() const {return data_directory;}
	std::string GetSharedDataPath() const {return data_directory+"/carparts";}
	std::string GetStartupFile() const {return settings_path+"/startingup.txt";}
	std::string GetTrackRecordsPath() const {return settings_path+"/records"+profile_suffix;}
	std::string GetSettingsFile() const {return settings_path+"/VDrift.config"+profile_suffix;}
	std::string GetLogFile() const {return settings_path+"/log.txt";}
	std::string GetTrackPath() const {return data_directory+"/"+GetTrackDir();}
	std::string GetCarPath() const {return data_directory+"/"+GetCarDir();}
	std::string GetGUIMenuPath(const std::string & skinname) const {return data_directory+"/skins/"+skinname+"/menus";}
	std::string GetSkinPath() const {return data_directory+"/skins/";}
	std::string GetOptionsFile() const {return data_directory + "/settings/options.config";}
	std::string GetVideoModeFile() const {return data_directory + "/lists/videomodes";}
	std::string GetCarControlsFile() const {return settings_path+"/controls.config"+profile_suffix;}
	std::string GetDefaultCarControlsFile() const {return data_directory+"/settings/controls.config";}
	std::string GetReplayPath() const {return settings_path+"/replays";}
	std::string GetScreenshotPath() const {return settings_path+"/screenshots";}
	std::string GetStaticReflectionMap() const {return data_directory+"/textures/weather/cubereflection-nosun.png";}
	std::string GetStaticAmbientMap() const {return data_directory+"/textures/weather/cubelighting.png";}
	std::string GetShaderPath() const {return data_directory + "/shaders";}
	std::string GetDataSettingsFile() const {return settings_path+"/data.config"+profile_suffix;}

	std::string GetTrackDir() const {return "tracks";}
	std::string GetCarDir() const {return "cars";}
	std::string GetCarSharedDir() const {return "carparts";}
	std::string GetGUITextureDir(const std::string & skinname) const {return "skins/"+skinname+"/textures";}
	std::string GetGUILanguageDir(const std::string & skinname) const {return "skins/"+skinname+"/languages";}
	std::string GetFontDir(const std::string & skinname) const {return "/skins/"+skinname+"/fonts";}
	std::string GetGenericSoundDir() const {return "sounds";}
	std::string GetHUDTextureDir() const {return "textures/hud";}
	std::string GetEffectsTextureDir() const {return "textures/effects";}
	std::string GetTireSmokeTextureDir() const {return "textures/smoke";}
	std::string GetDataLogPath() const {return settings_path+"/datalog";}

	bool FileExists(const std::string & filename) const;

	///<optionally filter for the given extension
	bool GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension="") const;

	///<only call this before Init()
	void SetProfile(const std::string & value);

	void Init(std::ostream & info_output, std::ostream & error_output);

private:
	std::string home_directory;
	std::string settings_path;
	std::string data_directory;
	std::string profile_suffix;
};

#endif
