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

#ifndef _SVN_SOURCEFORGE_H
#define _SVN_SOURCEFORGE_H

#include <string>
#include <map>

/// A cheesy HTML parser that mines sourceforge SVN web viewer pages to get repo info.
class SVN_SOURCEFORGE
{
public:
	/// Returns the download URL for any particular car given a name.
	static std::string GetCarDownloadLink(const std::string & dataurl, const std::string & carname);

	/// Given a sourceforge web svn folder view, return a map of folder names and revisions.
	std::map <std::string, int> ParseFolderView(const std::string & folderfile);
};

#endif
