#ifndef _HTTP_H
#define _HTTP_H

#include <string>
#include <set>
#include <map>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

struct HTTPINFO
{
	enum STATE
	{
		CONNECTING,
		DOWNLOADING,
		COMPLETE,
		FAILED
	} state;
	std::string error; ///< if empty, no error
	double totalsize; ///< bytes
	double downloaded; ///< bytes
	double speed; ///< bytes/second
	
	/// convert a state enum to a string
	static const char * GetString(STATE state);
	
	/// pretty-printing functions
	static std::string FormatSize(double bytes);
	static std::string FormatSpeed(double bytesPerSecond);
	
	bool operator!= (const HTTPINFO & other) const;
	bool operator== (const HTTPINFO & other) const;
	
	void print(std::ostream & s);
	
	HTTPINFO() : state(CONNECTING), totalsize(1),downloaded(0),speed(0) {}
};

/// for internal use only....
class HTTP;
struct PROGRESSINFO
{
	HTTP * http;
	CURL * easyhandle;
	PROGRESSINFO() : http(NULL), easyhandle(NULL) {}
	PROGRESSINFO(HTTP * newhttp, CURL * newhandle) : http(newhttp), easyhandle(newhandle) {}
};

/// This class manages one or many HTTP requests.
class HTTP
{
public:
	HTTP(const std::string & temporary_folder);
	~HTTP();
	
	void SetTemporaryFolder(const std::string & temporary_folder)
	{
		folder = temporary_folder;
	}
	
	/// returns true if the request succeeded, although note that this
	/// doesn't actually do any I/O operations until you call Tick()
	bool Request(const std::string & url, std::ostream & error_output);
	
	/// perform any I/O operations associated with any requests
	/// returns true if transfers are ongoing
	bool Tick();
	
	/// returns true if any requests are currently downloading
	bool Downloading() const {return downloading;}
	
	/// gets info about a request for a url.
	/// returns true if the url is for a known request.
	/// if the request is complete, then this function will
	/// remove the request from the info map, and further calls
	/// to this function with the same url will return false
	bool GetRequestInfo(const std::string & url, HTTPINFO & out);
	
	void CancelAllRequests();
	
	static std::string ExtractFilenameFromUrl(const std::string & url);
	
	std::string GetDownloadPath(const std::string & url);
	
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
	std::map <CURL*, REQUEST> easyhandles; ///< maps active curl easy handles to request info
	std::map <std::string, HTTPINFO> requests; ///< info about requests
};

#endif
