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

#include "http.h"
#include "unittest.h"
#include <cassert>

HttpInfo::HttpInfo() : state(CONNECTING), totalsize(1), downloaded(0), speed(0)
{
    // Constructor.
}

const char * HttpInfo::GetString(StateEnum state)
{
	switch (state)
	{
		case CONNECTING:
            return "connecting";
		case DOWNLOADING:
            return "downloading";
		case COMPLETE:
            return "complete";
		case FAILED:
            return "failed";
		default:
            return "error";
	};
}

std::string HttpInfo::FormatSize(double bytes)
{
	std::ostringstream s;
	s.precision(2);
	s << std::fixed;
	if (bytes < 0)
		s << "unknown";
	else if (bytes > 1000000)
		s << bytes/1000000 << "MB";
	else if (bytes > 1000)
		s << bytes/1000 << "KB";
	else
		s << bytes << "B";
	return s.str();
}

std::string HttpInfo::FormatSpeed(double bytes)
{
	std::ostringstream s;
	s.precision(2);
	s << std::fixed;
	if (bytes < 0)
		s << "unknown";
	else if (bytes > 1000000)
		s << bytes/1000000 << "MB/s";
	else if (bytes > 1000)
		s << bytes/1000 << "KB/s";
	else
		s << bytes << "B/s";
	return s.str();
}

bool HttpInfo::operator != (const HttpInfo & other) const
{
	return !(*this == other);
}

bool HttpInfo::operator == (const HttpInfo & other) const
{
    if (state != other.state) return false;
    if (totalsize != other.totalsize) return false;
    if (downloaded != other.downloaded) return false;
    if (error != other.error) return false;

	return true;
}

void HttpInfo::print(std::ostream & s)
{
	s << "State: " << GetString(state) << std::endl << "Total size: " << FormatSize(totalsize) << std::endl << "Downloaded: " << FormatSize(downloaded) << std::endl << "Speed: " << FormatSpeed(speed) << std::endl << "Error: " << (error.empty() ? "none" : error) << std::endl;
}

ProgressInfo::ProgressInfo() : http(NULL), easyhandle(NULL)
{
    // Cosntructor.
}

ProgressInfo::ProgressInfo(Http * newhttp, CURL * newhandle) : http(newhttp), easyhandle(newhandle)
{
    // Constructor.
}

// Just for storing curl initialization state.
static bool s_curl_init = false;

Http::Http(const std::string & temporary_folder) : folder(temporary_folder), downloading(false)
{
	if (!s_curl_init)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		s_curl_init = true;
	}

	// To use the multi interface, you must first create a 'multi handle'.
	multihandle = curl_multi_init();
}

Http::~Http()
{
	for (std::map <CURL*, RequestState>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
	{
		FILE * file = i->second.file;
		fclose(file);
		curl_easy_cleanup(i->first);
	}
	easyhandles.clear();
	requests.clear();

	if (multihandle)
		curl_multi_cleanup(multihandle);
	multihandle = NULL;
}

void Http::SetTemporaryFolder(const std::string & temporary_folder)
{
    folder = temporary_folder;
}

int ProgressCallback(void * ptr, double TotalToDownload, double NowDownloaded, double /*TotalToUpload*/, double /*NowUploaded*/)
{
	ProgressInfo * info = static_cast<ProgressInfo*>(ptr);
	assert(info->http && info->easyhandle);
	info->http->UpdateProgress(info->easyhandle, TotalToDownload, NowDownloaded);
	return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *)stream);
}

bool Http::Request(const std::string & url, std::ostream & error_output)
{
	if (!multihandle)
	{
		error_output << "HTTP::Request: multihandle initialization failed" << std::endl;
		return false;
	}

	// Each single transfer is built up with an easy handle.
	CURL * easyhandle = curl_easy_init();

	if (easyhandle)
	{
		// Setup the appropriate options for the easy handle.
		curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

		// This function call will make this multi_handle control the specified easy_handle.
		// Furthermore, libcurl now initiates the connection associated with the specified easy_handle.
		CURLMcode result = curl_multi_add_handle(multihandle, easyhandle);

		if (result == CURLM_OK)
		{
			// Open the destination file for write.
			std::string filepart = ExtractFilenameFromUrl(url);
			if (filepart.empty())
			{
				error_output << "HTTP::Request: url \"" << url << "\" is invalid" << std::endl;
				curl_multi_remove_handle(multihandle, easyhandle);
				return false;
			}

			std::string filename = folder+"/"+filepart;
			FILE * file = fopen(filename.c_str(),"wb");
			if (!file)
			{
				error_output << "HTTP::Request: unable to open \"" << filename << "\" for writing" << std::endl;
				curl_multi_remove_handle(multihandle, easyhandle);
				return false;
			}

			// Setup file writing.
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, file);

			// Begin tracking the easyhandle.
			RequestState requestinfo(url,file);
			requestinfo.progress_callback_data.http = this;
			requestinfo.progress_callback_data.easyhandle = easyhandle;
			easyhandles.insert(std::make_pair(easyhandle,requestinfo));
			requests.insert(std::make_pair(url,HttpInfo()));

			// Setup the progress callback.
			curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
			curl_easy_setopt(easyhandle, CURLOPT_PROGRESSDATA, &(easyhandles.find(easyhandle)->second.progress_callback_data));

			return true;
		}
		else
		{
			// Tell the multihandle to forget the handle.
			curl_multi_remove_handle(multihandle, easyhandle);

			error_output << "HTTP::Request: CURL is unable to request URL \"" << url << "\"" << std::endl;
			return false;
		}
	}
	else
	{
		error_output << "HTTP::Request: easyhandle initialization failed" << std::endl;
		return false;
	}
}

bool Http::Tick()
{
	// curl_multi_perform() returns as soon as the reads/writes are done.
	// This function does not require that there actually is any data available for reading or that data can be written, it can be called just in case.
	int running_transfers = 0;
	int loopcheck = 0;
	CURLMcode result = CURLM_CALL_MULTI_PERFORM;
	while (result == CURLM_CALL_MULTI_PERFORM)
	{
		result = curl_multi_perform(multihandle, &running_transfers);
		loopcheck++;
		assert(loopcheck < 1000 && "infinite loop in HTTP::Tick()");
	}

	CURLMsg * msg = NULL;
	do
	{
		int msgs_in_queue = 0;
		msg = curl_multi_info_read(multihandle, &msgs_in_queue);
		if (msg && msg->msg == CURLMSG_DONE)
		{
			// Handle completion.
			CURL * easyhandle = msg->easy_handle;

			// Get the url.
			std::map <CURL*, RequestState>::iterator u = easyhandles.find(easyhandle);
			assert(u != easyhandles.end() && "corruption in easyhandles map");
			std::string url = u->second.url;

			if (msg->data.result == CURLE_OK)
			{
				// Completion.
				requests[url].state = HttpInfo::COMPLETE;
				curl_easy_getinfo(easyhandle, CURLINFO_SPEED_DOWNLOAD, &requests[url].speed);
			}
			else
			{
				// Failure.
				requests[url].state = HttpInfo::FAILED;
				requests[url].error = "unknown";
			}

			// Cleanup.
			curl_easy_cleanup(easyhandle);
			fclose(u->second.file);
			easyhandles.erase(easyhandle);
		}

		// Update status.
		for (std::map <CURL*, RequestState>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
		{
			CURL * easyhandle = i->first;
			std::map <CURL*, RequestState>::iterator u = easyhandles.find(easyhandle);
			assert(u != easyhandles.end() && "corruption in requestUrls map");
			std::string url = u->second.url;

			curl_easy_getinfo(easyhandle, CURLINFO_SPEED_DOWNLOAD, &requests[url].speed);
			requests[url].state = requests[url].downloaded > 0 ? HttpInfo::DOWNLOADING : HttpInfo::CONNECTING;
		}
	}
	while (msg);

	downloading = (running_transfers > 0);

	return downloading;
}

bool Http::Downloading() const
{
    return downloading;
}

bool Http::GetRequestInfo(const std::string & url, HttpInfo & out)
{
	std::map <std::string, HttpInfo>::iterator i = requests.find(url);
	if (i == requests.end())
		return false;

	out = i->second;

	if (i->second.state == HttpInfo::FAILED || i->second.state == HttpInfo::COMPLETE)
		requests.erase(i);

	return true;
}

void Http::CancelAllRequests()
{
	for (std::map <CURL*, RequestState>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
	{
		FILE * file = i->second.file;
		fclose(file);
		curl_easy_cleanup(i->first);
	}
	easyhandles.clear();
	requests.clear();
	downloading = false;
}

std::string Http::ExtractFilenameFromUrl(const std::string & in)
{
	std::string url = in;
	size_t start = url.find_last_of('/');
	size_t end = url.find_first_of('?');
	if (start == std::string::npos)
		start = 0;
	else
		start++;
	if (end == std::string::npos)
		end = url.size();
	if (end > 1 && url[end-1] == '/')
	{
		url[end-1] = '?';
		start = url.find_last_of('/');
		end = url.find_first_of('?');
		if (start == std::string::npos)
			start = 0;
		else
			start++;
	}
	return url.substr(start,end-start);
}

std::string Http::GetDownloadPath(const std::string & url) const
{
	return folder+"/"+ExtractFilenameFromUrl(url);
}

void Http::UpdateProgress(CURL * handle, float total, float current)
{
	std::map <CURL*, RequestState>::iterator i = easyhandles.find(handle);
	if (i == easyhandles.end())
		return;

	std::string url = i->second.url;
	requests[url].totalsize = total;
	requests[url].downloaded = current;
}

QT_TEST(http)
{
	Http http("data/test");
	const bool verbose = false;

	{
		std::string testurl = "vdrift.net";
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.Request(testurl, std::cerr));
		HttpInfo lastinfo;
		HttpInfo curinfo;
		while (http.Tick())
		{
			QT_CHECK(http.GetRequestInfo(testurl, curinfo));
			QT_CHECK(http.Downloading());
			if (curinfo != lastinfo && verbose)
			{
				curinfo.print(std::cout);
				std::cout << std::endl;
			}
			lastinfo = curinfo;
		}
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.GetRequestInfo(testurl, curinfo));
		if (verbose) curinfo.print(std::cout);
		QT_CHECK(!http.GetRequestInfo(testurl, curinfo));
	}

	{
		std::string testurl = "badurl";
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.Request(testurl, std::cerr));
		HttpInfo lastinfo;
		HttpInfo curinfo;
		while (http.Tick())
		{
			QT_CHECK(http.GetRequestInfo(testurl, curinfo));
			QT_CHECK(http.Downloading());
			lastinfo = curinfo;
		}
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.GetRequestInfo(testurl, curinfo));
		QT_CHECK_EQUAL(curinfo.state,HttpInfo::FAILED);
		QT_CHECK(!http.GetRequestInfo(testurl, curinfo));
	}

	// check filename extraction from url
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("HTTP://vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://vdrift.net/test"), std::string("test"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("vdrift.net/test"), std::string("test"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/second"), std::string("second"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt"), std::string("second.txt"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt?"), std::string("second.txt"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt?stuff=hello"), std::string("second.txt"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/e"), std::string("e"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/e?"), std::string("e"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/e???"), std::string("e"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/"), std::string("test"));
	QT_CHECK_EQUAL(Http::ExtractFilenameFromUrl("http://www.vdrift.net/test/?aoeu"), std::string("test"));
}

