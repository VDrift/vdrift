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

#include <sstream>
#include "svn_sourceforge.h"
#include "utils.h"
#include "http.h"
#include "unittest.h"

std::string SVN_SOURCEFORGE::GetCarFolderUrl() const
{
	return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/cars/";
}

std::string SVN_SOURCEFORGE::GetCarDownloadLink(const std::string & car)
{
	return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/cars/"+car+"/?view=tar";
}

std::string SVN_SOURCEFORGE::GetRemoteUpdateConfigUrl()
{
	return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/settings/updates.config";
}

std::map <std::string, int> SVN_SOURCEFORGE::ParseFolderView(const std::string & folderfile)
{
	std::map <std::string, int> folders;

	std::stringstream s(folderfile);

	// Fast forward to the start of the list.
	UTILS::SeekTo(s,"&nbsp;Parent&nbsp;Directory");

	// Loop through each entry.
	while (s)
	{
		UTILS::SeekTo(s,"<a name=\"");
		std::string name = UTILS::SeekTo(s,"\"");
		UTILS::SeekTo(s,"title=\"View directory revision log\"><strong>");
		std::string revstr = UTILS::SeekTo(s,"</strong>");

		if (name != "SConscript" && !name.empty() && !revstr.empty())
		{
			std::stringstream converter(revstr);
			int rev(0);
			converter >> rev;
			if (rev != 0)
				folders[name] = rev;
		}
	}

	return folders;
}

QT_TEST(svn_sourceforge)
{
	HTTP http("data/test");
	SVN_SOURCEFORGE svn;
	std::string testurl = svn.GetCarFolderUrl();

	QT_CHECK(http.Request(testurl, std::cerr));
	HTTPINFO curinfo;
	while (http.Tick());
	QT_CHECK(http.GetRequestInfo(testurl, curinfo));
	QT_CHECK(curinfo.state == HTTPINFO::COMPLETE);

	std::string page = UTILS::LoadFileIntoString(http.GetDownloadPath(testurl), std::cerr);
	std::map <std::string, int> res = svn.ParseFolderView(page);

	QT_CHECK(res.size() > 10);
}
