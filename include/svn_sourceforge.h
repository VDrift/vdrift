#ifndef _SVN_SOURCEFORGE_H
#define _SVN_SOURCEFORGE_H

#include <string>
#include <map>

/// A cheesy HTML parser that mines sourceforge SVN web viewer pages to get repo info
class SVN_SOURCEFORGE
{
public:
	std::string GetCarFolderUrl() const {return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/cars/";}
	static std::string GetCarDownloadLink(const std::string & car) {return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/cars/"+car+"/?view=tar";}
	static std::string GetRemoteUpdateConfigUrl() {return "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/settings/updates.config";}

	/// given a sourceforge web svn folder view, return a map of folder names and revisions
	std::map <std::string, int> ParseFolderView(const std::string & folderfile);

};

#endif
