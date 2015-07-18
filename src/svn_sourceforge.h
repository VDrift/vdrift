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

#ifndef _SVN_SOURCE_FORGE_H
#define _SVN_SOURCE_FORGE_H

#include <iosfwd>
#include <string>
#include <map>

class GameDownloader;

namespace SvnSourceForge
{
	/// Given a sourceforge web svn folder view, return a map of folder names and revisions.
	std::map <std::string, int> ParseFolderView(std::istream & page);

	/// Given a sourceforge file folder url and download folder path,
	/// download the folder incusive all files. Return fase on error.
	bool DownloadFolder(
		const std::string & page_url,
		const std::string & folder_path,
		GameDownloader & download,
		std::ostream & error_output);
}

#endif
