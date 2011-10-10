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

#ifndef _HTTP_H
#define _HTTP_H

#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <map>

struct HTTPINFO
{
    HTTPINFO();

	enum STATE
	{
		CONNECTING,
		DOWNLOADING,
		COMPLETE,
		FAILED
	} state;

	std::string error; ///< If empty, no error.
	double totalsize; ///< Total file size in bytes.
	double downloaded; ///< Downloaded part in bytes.
	double speed; ///< Downalod speed in bytes per second.

	/// Convert a state enum to a string.
	static const char * GetString(STATE state);

	/// Pretty-printing functions.
	static std::string FormatSize(double bytes);
	static std::string FormatSpeed(double bytesPerSecond);

	bool operator != (const HTTPINFO & other) const;
	bool operator == (const HTTPINFO & other) const;

	void print(std::ostream & s);
};

/// For internal use only.
class HTTP;
struct PROGRESSINFO
{
    PROGRESSINFO();
	PROGRESSINFO(HTTP * newhttp, CURL * newhandle);
	HTTP * http;
	CURL * easyhandle;
};

/// This class manages one or many HTTP requests.
class HTTP
{
public:
	HTTP(const std::string & temporary_folder);
	~HTTP();

    /// Change temporary folder.
	void SetTemporaryFolder(const std::string & temporary_folder);

	/// Returns true if the request succeeded, although note that this doesn't actually do any I/O operations until you call Tick().
	bool Request(const std::string & url, std::ostream & error_output);

	/// Perform any I/O operations associated with any requests.
	/// Returns true if transfers are ongoing.
	bool Tick();

	/// Returns true if any requests are currently downloading.
	bool Downloading() const;

	/// Gets info about a request for a url.
	/// Returns true if the url is for a known request.
	/// If the request is complete, then this function will remove the request from the info map and further calls to this function with the same url will return false
	bool GetRequestInfo(const std::string & url, HTTPINFO & out);

	void CancelAllRequests();

	static std::string ExtractFilenameFromUrl(const std::string & url);

	std::string GetDownloadPath(const std::string & url) const;

	void UpdateProgress(CURL * handle, float total, float current);

private:
	std::string folder;
	bool downloading;
	CURLM * multihandle;
	struct REQUEST
	{
		std::string url;
		FILE * file;
		PROGRESSINFO progress_callback_data;
		REQUEST(const std::string & newurl, FILE * newfile) : url(newurl), file(newfile) {}
	};
	std::map <CURL*, REQUEST> easyhandles; ///< Maps active curl easy handles to request info.
	std::map <std::string, HTTPINFO> requests; ///< Info about requests.
};

#endif
