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

std::string SvnSourceforge::GetDownloadLink(const std::string & dataurl, const std::string & group, const std::string & name)
{
	return dataurl + group + "/" + name + "/?view=tar";
}

std::map <std::string, int> SvnSourceforge::ParseFolderView(const std::string & folderfile)
{
	std::map <std::string, int> folders;

	std::stringstream s(folderfile);

	// Fast forward to the start of the list.
	Utils::SeekTo(s, "<tbody>");

	// Loop through each entry.
	while (s)
	{
		Utils::SeekTo(s, "\"ico folder\"></b>\n          <span>");
		const std::string name = Utils::SeekTo(s, "<");

		Utils::SeekTo(s, "[r");
		const std::string revstr = Utils::SeekTo(s, "]");

		if (!name.empty() && !revstr.empty())
		{
			std::stringstream converter(revstr);
			int rev = 0;
			converter >> rev;
			if (rev != 0)
				folders[name] = rev;
		}
	}

	return folders;
}

QT_TEST(svn_sourceforge)
{
	Http http("data/test");
	SvnSourceforge svn;
	const std::string url = "http://sourceforge.net/p/vdrift/code/HEAD/tree/vdrift-data/cars/";

	QT_CHECK(http.Request(url, std::cerr));
	HttpInfo curinfo;
	while (http.Tick());
	QT_CHECK(http.GetRequestInfo(url, curinfo));
	QT_CHECK(curinfo.state == HttpInfo::COMPLETE);

	std::string page = Utils::LoadFileIntoString(http.GetDownloadPath(url), std::cerr);
	std::map <std::string, int> res = svn.ParseFolderView(page);

	QT_CHECK(res.size() > 10);
}
