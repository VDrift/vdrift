#include "http.h"
#include "unittest.h"

#include <sstream>
#include <cstdio>
#include <cassert>

static bool s_curl_init = false;

HTTP::HTTP(const std::string & temporary_folder) : folder(temporary_folder), downloading(false)
{
	if (!s_curl_init)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		s_curl_init = true;
	}

	// To use the multi interface, you must first create a 'multi handle'
	multihandle = curl_multi_init();
}

int ProgressCallback(void * ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	PROGRESSINFO * info = (PROGRESSINFO*)ptr;
	assert(info->http && info->easyhandle);
	info->http->UpdateProgress(info->easyhandle, TotalToDownload, NowDownloaded);
	return 0;
}

std::string HTTP::ExtractFilenameFromUrl(const std::string & in)
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

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *)stream);
}

bool HTTP::Request(const std::string & url, std::ostream & error_output)
{
	if (!multihandle)
	{
		error_output << "HTTP::Request: multihandle initialization failed" << std::endl;
		return false;
	}

	// Each single transfer is built up with an easy handle
	CURL * easyhandle = curl_easy_init();

	if (easyhandle)
	{
		// setup the appropriate options for the easy handle
		curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());

		// This function call will make this multi_handle control the specified easy_handle. 
		// Furthermore, libcurl now initiates the connection associated with the specified easy_handle.
		CURLMcode result = curl_multi_add_handle(multihandle, easyhandle);

		if (result == CURLM_OK)
		{
			// open the destination file for write
			std::string filepart = ExtractFilenameFromUrl(url);
			if (filepart.empty())
			{
				error_output << "HTTP::Request: url \"" << url << "\" is invalid" << std::endl;
				curl_multi_remove_handle(multihandle, easyhandle);
				return false;
			}

			std::string filename = folder+"/"+filepart;
			FILE * file = fopen(filename.c_str(),"w");
			if (!file)
			{
				error_output << "HTTP::Request: unable to open \"" << filename << "\" for writing" << std::endl;
				curl_multi_remove_handle(multihandle, easyhandle);
				return false;
			}

			// setup file writing
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, file);

			// begin tracking the easyhandle
			REQUEST requestinfo(url,file);
			requestinfo.progress_callback_data.http = this;
			requestinfo.progress_callback_data.easyhandle = easyhandle;
			easyhandles.insert(std::make_pair(easyhandle,requestinfo));
			requests.insert(std::make_pair(url,HTTPINFO()));

			// setup the progress callback
			curl_easy_setopt(easyhandle, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
			curl_easy_setopt(easyhandle, CURLOPT_PROGRESSDATA, &(easyhandles.find(easyhandle)->second.progress_callback_data));

			return true;
		}
		else
		{
			// tell the multihandle to forget the handle
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

HTTP::~HTTP()
{
	for (std::map <CURL*, REQUEST>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
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

void HTTP::CancelAllRequests()
{
	for (std::map <CURL*, REQUEST>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
	{
		FILE * file = i->second.file;
		fclose(file);
		curl_easy_cleanup(i->first);
	}
	easyhandles.clear();
	requests.clear();
	downloading = false;
}

void HTTP::UpdateProgress(CURL * handle, float total, float current)
{
	std::map <CURL*, REQUEST>::iterator i = easyhandles.find(handle);
	if (i == easyhandles.end())
		return;

	std::string url = i->second.url;
	requests[url].totalsize = total;
	requests[url].downloaded = current;
}

bool HTTP::Tick()
{
	// curl_multi_perform() returns as soon as the reads/writes are done. 
	// This function does not require that there actually is any data available for reading or that data 
	// can be written, it can be called just in case.
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
			// handle completion
			CURL * easyhandle = msg->easy_handle;

			// get the url
			std::map <CURL*, REQUEST>::iterator u = easyhandles.find(easyhandle);
			assert(u != easyhandles.end() && "corruption in easyhandles map");
			std::string url = u->second.url;

			if (msg->data.result == CURLE_OK)
			{
				// completion
				requests[url].state = HTTPINFO::COMPLETE;
				curl_easy_getinfo(easyhandle, CURLINFO_SPEED_DOWNLOAD, &requests[url].speed);
			}
			else
			{
				// failure
				requests[url].state = HTTPINFO::FAILED;
				requests[url].error = "unknown";
			}

			// cleanup
			curl_easy_cleanup(easyhandle);
			fclose(u->second.file);
			easyhandles.erase(easyhandle);
		}

		// update status
		for (std::map <CURL*, REQUEST>::iterator i = easyhandles.begin(); i != easyhandles.end(); i++)
		{
			CURL * easyhandle = i->first;
			std::map <CURL*, REQUEST>::iterator u = easyhandles.find(easyhandle);
			assert(u != easyhandles.end() && "corruption in requestUrls map");
			std::string url = u->second.url;

			curl_easy_getinfo(easyhandle, CURLINFO_SPEED_DOWNLOAD, &requests[url].speed);
			requests[url].state = requests[url].downloaded > 0 ? HTTPINFO::DOWNLOADING : HTTPINFO::CONNECTING;
		}
	}
	while (msg);

	downloading = (running_transfers > 0);

	return downloading;
}

void HTTPINFO::print(std::ostream & s)
{
	s << "State: " << GetString(state) << std::endl;
	s << "Total size: " << FormatSize(totalsize) << std::endl;
	s << "Downloaded: " << FormatSize(downloaded) << std::endl;
	s << "Speed: " << FormatSpeed(speed) << std::endl;
	s << "Error: " << (error.empty() ? "none" : error) << std::endl;
}

bool HTTPINFO::operator!=(const HTTPINFO & other) const
{
	return !(*this == other);
}

bool HTTPINFO::operator==(const HTTPINFO & other) const
{
#define X(x) if (x != other.x) return false
	X(state);
	X(totalsize);
	X(downloaded);
	// speed is purposely ignored
	X(error);
#undef X

	return true;
}

bool HTTP::GetRequestInfo(const std::string & url, HTTPINFO & out)
{
	std::map <std::string, HTTPINFO>::iterator i = requests.find(url);
	if (i == requests.end())
	{
		return false;
	}

	out = i->second;

	if (i->second.state == HTTPINFO::FAILED || i->second.state == HTTPINFO::COMPLETE)
		requests.erase(i);

	return true;
}

const char * HTTPINFO::GetString(STATE state)
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

std::string HTTPINFO::FormatSize(double bytes)
{
	std::stringstream s;
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

std::string HTTPINFO::FormatSpeed(double bytes)
{
	std::stringstream s;
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

std::string HTTP::GetDownloadPath(const std::string & url)
{
	return folder+"/"+ExtractFilenameFromUrl(url);
}

QT_TEST(http)
{
	HTTP http("data/test");
	const bool verbose = false;

	{
		std::string testurl = "vdrift.net";
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.Request(testurl, std::cerr));
		HTTPINFO lastinfo;
		HTTPINFO curinfo;
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
		HTTPINFO lastinfo;
		HTTPINFO curinfo;
		while (http.Tick())
		{
			QT_CHECK(http.GetRequestInfo(testurl, curinfo));
			QT_CHECK(http.Downloading());
			lastinfo = curinfo;
		}
		QT_CHECK(!http.Downloading());
		QT_CHECK(http.GetRequestInfo(testurl, curinfo));
		QT_CHECK_EQUAL(curinfo.state,HTTPINFO::FAILED);
		QT_CHECK(!http.GetRequestInfo(testurl, curinfo));
	}

	// check filename extraction from url
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("HTTP://vdrift.net"), std::string("vdrift.net"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://vdrift.net/test"), std::string("test"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("vdrift.net/test"), std::string("test"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/second"), std::string("second"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt"), std::string("second.txt"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt?"), std::string("second.txt"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/second.txt?stuff=hello"), std::string("second.txt"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/e"), std::string("e"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/e?"), std::string("e"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/e???"), std::string("e"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/"), std::string("test"));
	QT_CHECK_EQUAL(HTTP::ExtractFilenameFromUrl("http://www.vdrift.net/test/?aoeu"), std::string("test"));
}

