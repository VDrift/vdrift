#include "svn_sourceforge.h"

#include "utils.h"
#include "http.h"
#include "unittest.h"

#include <sstream>

std::map <std::string, int> SVN_SOURCEFORGE::ParseFolderView(const std::string & folderfile)
{
	std::map <std::string, int> folders;
	
	std::stringstream s(folderfile);
	
	// fast forward to the start of the list
	UTILS::SeekTo(s,"&nbsp;Parent&nbsp;Directory");
	
	// loop through each entry
	while (s)
	{
		UTILS::SeekTo(s,"<a name=\"");
		std::string name = UTILS::SeekTo(s,"\"");
		UTILS::SeekTo(s,"title=\"View directory revision log\"><strong>");
		std::string revstr = UTILS::SeekTo(s,"</strong>");
		
		//std::cout << "name: " << name << ", revstr: " << revstr << std::endl;
		
		if (name != "SConscript" && !name.empty() && !revstr.empty())
		{
			std::stringstream converter(revstr);
			int rev(0);
			converter >> rev;
			if (rev != 0)
			{
				folders[name] = rev;
			}
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
	while (http.Tick())
	{
	}
	QT_CHECK(http.GetRequestInfo(testurl, curinfo));
	QT_CHECK(curinfo.state == HTTPINFO::COMPLETE);
	
	std::string page = UTILS::LoadFileIntoString(http.GetDownloadPath(testurl), std::cerr);
	std::map <std::string, int> res = svn.ParseFolderView(page);
	
	/*std::cout << "svn_sourceforge test result: " << res.size() << std::endl;
	for (std::map <std::string, int>::const_iterator i = res.begin(); i != res.end(); i++)
	{
		std::cout << "\t" << i->first << ", " << i->second << std::endl;
	}*/
}
