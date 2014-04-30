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
/* This is the main entry point for VDrift.                             */
/*                                                                      */
/************************************************************************/

#include "svn_sourceforge.h"
#include "game_downloader.h"
#include "pathmanager.h"
#include "http.h"
#include "utils.h"
#include "unittest.h"

#include <fstream>
#include <sstream>

// Parse rep info from sourceforge svn web view.
std::map <std::string, int> SvnSourceForge::ParseFolderView(std::istream & page)
{
	std::map <std::string, int> folders;

	// Fast forward to the start of the list.
	Utils::SeekTo(page, "<tbody>");

	// Loop through each entry.
	while (page)
	{
		Utils::SeekTo(page, "\"ico folder\"></b>\n          <span>");
		const std::string name = Utils::SeekTo(page, "<");

		Utils::SeekTo(page, "[r");
		const std::string revstr = Utils::SeekTo(page, "]");

		if (name.empty() || revstr.empty())
			continue;

		std::stringstream converter(revstr);
		int rev = 0;
		converter >> rev;
		if (rev != 0)
			folders[name] = rev;
	}

	return folders;
}

// Download files from sourceforge svn web view page, preserve folder structure.
bool SvnSourceForge::DownloadFolder(
	const std::string & page_url,
	const std::string & folder_path,
	GameDownloader & download,
	std::ostream & error_output)
{
	// Ensure the folder exists.
	PathManager::MakeDir(folder_path);

	// Download folder page.
	if (!download(page_url))
	{
		error_output << "Download failed: " << page_url << std::endl;
		return false;
	}

	// Load folder page.
	const std::string page_path = download.GetHttp().GetDownloadPath(page_url);
	std::ifstream page(page_path.c_str());
	if (!page)
	{
		error_output << "File not found: " << page_path << std::endl;
		return false;
	}

	// Fast forward to the start of the list.
	Utils::SeekTo(page, "<ul>");

	// Loop through each entry.
	std::vector<std::string> names;
	while (page)
	{
		Utils::SeekTo(page, "<li><a href=\"");
		const std::string name = Utils::SeekTo(page, "\">");

		if (name.empty() || name == "../" || name == "Sconscript")
			continue;

		if (*name.rbegin() == '/')
		{
			// Download sub folder.
			if (!DownloadFolder(page_url + name, folder_path + name, download, error_output))
			{
				while (!names.empty())
				{
					PathManager::RemoveFile(folder_path + names.back());
					names.pop_back();
				}
				PathManager::RemoveDir(folder_path);
				return false;
			}
			continue;
		}

		// Download file.
		names.push_back(name);
		const std::string file_url = page_url + name;
		if (!download(file_url))
		{
			error_output << "Download failed: " << file_url << std::endl;
			while (!names.empty())
			{
				PathManager::RemoveFile(folder_path + names.back());
				names.pop_back();
			}
			PathManager::RemoveDir(folder_path);
			return false;
		}

		// Copy file into folder.
		const std::string temp_path = download.GetHttp().GetDownloadPath(file_url);
		const std::string file_path = folder_path + name;
		PathManager::CopyFileTo(temp_path, file_path);
		PathManager::RemoveFile(temp_path);
	}

	page.close();
	PathManager::RemoveFile(page_path);
	return true;
}

QT_TEST(svn_source_forge)
{
	const std::string url = "http://sourceforge.net/p/vdrift/code/HEAD/tree/vdrift-data/cars/";

	Http http("data/test");
	QT_CHECK(http.Request(url, std::cerr));

	HttpInfo curinfo;
	while (http.Tick());
	QT_CHECK(http.GetRequestInfo(url, curinfo));
	QT_CHECK(curinfo.state == HttpInfo::COMPLETE);

	std::ifstream page(http.GetDownloadPath(url).c_str());
	QT_CHECK(page);

	const std::map <std::string, int> res = SvnSourceForge::ParseFolderView(page);
	QT_CHECK(res.size() > 10);
}
