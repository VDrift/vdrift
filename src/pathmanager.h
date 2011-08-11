#ifndef _PATHMANAGER_H
#define _PATHMANAGER_H

#include <list>
#include <string>

class PATHMANAGER
{
public:
	std::string GetDataPath() const {return data_directory;}
	std::string GetWriteableDataPath() const {return settings_path;}
	std::string GetSharedCarPath() const {return GetDataPath()+"/carparts";}
	std::string GetSharedTrackPath() const {return GetDataPath()+"/trackparts";}
	std::string GetStartupFile() const {return settings_path+"/startingup.txt";}
	std::string GetTrackRecordsPath() const {return settings_path+"/records"+profile_suffix;}
	std::string GetSettingsFile() const {return settings_path+"/VDrift.config"+profile_suffix;}
	std::string GetLogFile() const {return settings_path+"/log.txt";}
	std::string GetTrackPath() const {return GetDataPath()+"/"+GetTrackDir();}
	std::string GetCarPath(const std::string & carname) const;
	std::string GetCarPaintPath(const std::string & carname) const {return GetCarPath(carname)+"/skins";}
	std::string GetGUIMenuPath(const std::string & skinname) const {return GetDataPath()+"/skins/"+skinname+"/menus";}
	std::string GetSkinPath() const {return GetDataPath()+"/skins/";}
	std::string GetOptionsFile() const {return GetDataPath() + "/settings/options.config";}
	std::string GetVideoModeFile() const {return GetDataPath() + "/lists/videomodes";}
	std::string GetCarControlsFile() const {return settings_path+"/controls.config"+profile_suffix;}
	std::string GetDefaultCarControlsFile() const {return GetDataPath()+"/settings/controls.config";}
	std::string GetReplayPath() const {return settings_path+"/replays";}
	std::string GetScreenshotPath() const {return settings_path+"/screenshots";}
	std::string GetStaticReflectionMap() const {return GetDataPath()+"/textures/weather/cubereflection-nosun.png";}
	std::string GetStaticAmbientMap() const {return GetDataPath()+"/textures/weather/cubelighting.png";}
	std::string GetShaderPath() const {return GetDataPath() + "/shaders";}
	std::string GetUpdateManagerFile() const {return settings_path+"/updates.config"+profile_suffix;}
	std::string GetUpdateManagerFileBackup() const {return settings_path+"/updates.config.backup"+profile_suffix;}
	std::string GetUpdateManagerFileBase() const {return GetDataPath() + "/settings/updates.config";}

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
	
	std::string GetReadOnlyCarPath() const {return GetDataPath()+"/"+GetCarDir();}
	std::string GetWriteableCarPath() const {return GetWriteableDataPath()+"/"+GetCarDir();}

	std::string GetTemporaryFolder() const {return temporary_folder;}

	bool FileExists(const std::string & filename) const;

	///<optionally filter for the given extension
	bool GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension="") const;

	///<only call this before Init()
	void SetProfile(const std::string & value);

	void Init(std::ostream & info_output, std::ostream & error_output);
	
	static void MakeDir(const std::string & dir);
	static void DeleteFile1(const std::string & path);

private:
	std::string home_directory;
	std::string settings_path;
	std::string data_directory;
	std::string profile_suffix;
	std::string temporary_folder;
};

#endif
