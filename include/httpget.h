#ifndef _HTTPGET_H
#define _HTTPGET_H

#include <ostream>
#include <string>

#include <asio.hpp>
#include <boost/bind.hpp>

namespace httpget
{

///returns true on success.  puts the output in the provided string.  blocks.
bool Get(const std::string & server, const std::string & path, std::ostream & result, std::ostream & error_output);

class Getter
{
	public:
		enum STATUS
		{
			STATE_INWORK,
			STATE_ERROR,
			STATE_COMPLETE
		};
		
	private:
		asio::io_service io_service_;
		asio::ip::tcp::resolver resolver_;
		asio::ip::tcp::socket socket_;
		asio::streambuf request_;
		asio::streambuf response_;
		std::stringstream result_;
		STATUS state;
		std::stringstream error_output_;
		
		void handle_resolve(const asio::error_code& err, asio::ip::tcp::resolver::iterator endpoint_iterator);
		void handle_connect(const asio::error_code& err, asio::ip::tcp::resolver::iterator endpoint_iterator);
		void handle_write_request(const asio::error_code& err);
		void handle_read_status_line(const asio::error_code& err);
		void handle_read_headers(const asio::error_code& err);
		void handle_read_content(const asio::error_code& err);
		void Error();
		
	public:
		Getter(const std::string& server, const std::string& path);
		std::string GetResult() const; ///< if the result isn't available (GetStatus() != COMPLETE), an empty string is returned.
		STATUS GetStatus();
		std::string GetError() const {return error_output_.str();}
};

};

#endif
