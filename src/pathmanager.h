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

#ifndef _PATHMANAGER_H
#define _PATHMANAGER_H

#include <list>
#include <iosfwd>
#include <string>

class PathManager
{
public:
	void Init(std::ostream & info_output, std::ostream & error_output);

	/// Only call this before Init().
	void SetProfile(const std::string & value);

	/// Optionally filter for the given extension.
	bool GetFileList(std::string folderpath, std::list <std::string> & outputfolderlist, std::string extension="") const;

	bool FileExists(const std::string & filename) const;
	static void CopyFileTo(const std::string & oldname, const std::string & newname);
	static void MakeDir(const std::string & dir);
	static void RemoveDir(const std::string & dir);
	static void RemoveFile(const std::string & path);

	std::string GetDataPath() const;
	std::string GetWriteableDataPath() const;
	std::string GetCarPartsPath() const;
	std::string GetTrackPartsPath() const;
	std::string GetStartupFile() const;
	std::string GetTrackRecordsPath() const;
	std::string GetSettingsFile() const;
	std::string GetLogFile() const;
	std::string GetTracksPath(const std::string & carname) const;
	std::string GetCarPath(const std::string & carname) const;
	std::string GetCarPaintPath(const std::string & carname) const;
	std::string GetGUIMenuPath(const std::string & skinname) const;
	std::string GetSkinsPath() const;
	std::string GetOptionsFile() const;
	std::string GetCarControlsFile() const;
	std::string GetDefaultCarControlsFile() const;
	std::string GetReplayPath() const;
	std::string GetScreenshotPath() const;
	std::string GetStaticReflectionMap() const;
	std::string GetStaticAmbientMap() const;
	std::string GetShaderPath() const;
	std::string GetUpdateManagerFile() const;
	std::string GetUpdateManagerFileBackup() const;
	std::string GetUpdateManagerFileBase() const;

	std::string GetTracksDir() const;
	std::string GetCarsDir() const;
	std::string GetCarPartsDir() const;
	std::string GetSkinsDir() const;
	std::string GetGUITextureDir(const std::string & skinname) const;
	std::string GetGUILanguageDir(const std::string & skinname) const;
	std::string GetFontDir(const std::string & skinname) const;
	std::string GetGenericSoundDir() const;
	std::string GetHUDTextureDir() const;
	std::string GetEffectsTextureDir() const;
	std::string GetTireSmokeTextureDir() const;

	std::string GetReadOnlyCarsPath() const;
	std::string GetWriteableCarsPath() const;
	std::string GetReadOnlyTracksPath() const;
	std::string GetWriteableTracksPath() const;

	std::string GetTemporaryFolder() const;

private:
	std::string home_directory;
	std::string settings_path;
	std::string data_directory;
	std::string profile_suffix;
	std::string temporary_folder;
};

#endif
