#include "httpget.h"
#include "unittest.h"

using std::endl;
using boost::asio::ip::tcp;

namespace httpget
{

bool Get(const std::string & server, const std::string & path, std::ostream & result, std::ostream & error_output)
{
	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(server, "http");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && endpoint_iterator != end)
	{
		socket.close();
		socket.connect(*endpoint_iterator++, error);
	}
	if (error)
	{
		error_output << "Unable to establish connection" << endl;
		return false;
	}

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << path << " HTTP/1.0\r\n";
	request_stream << "Host: " << server << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	// Send the request.
	boost::asio::write(socket, request);

	// Read the response status line.
	boost::asio::streambuf response;
	boost::asio::read_until(socket, response, "\r\n");

	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	unsigned int status_code;
	response_stream >> status_code;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/")
	{
		error_output << "Invalid response" << endl;
		return false;
	}
	if (status_code != 200)
	{
		error_output << "Response returned with status code " << status_code << endl;
		return false;
	}

	// Read the response headers, which are terminated by a blank line.
	boost::asio::read_until(socket, response, "\r\n\r\n");

	// Process the response headers.
	std::string header;
	while (std::getline(response_stream, header) && header != "\r")
	{
		//result << header << "\n";
	}
	//result << "\n";

	// Write whatever content we already have to output.
	if (response.size() > 0)
		result << &response;

	// Read until EOF, writing data to output as we go.
	while (boost::asio::read(socket, response,
		boost::asio::transfer_at_least(1), error))
		result << &response;
	if (error != boost::asio::error::eof)
	{
		error_output << "Unexpected transfer error" << endl;
		return false;
	}
	
	return true;
}

void Getter::handle_resolve(const boost::system::error_code& err,
							tcp::resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		// Attempt a connection to the first endpoint in the list. Each endpoint
		// will be tried until we successfully establish a connection.
		tcp::endpoint endpoint = *endpoint_iterator;
		socket_.async_connect(endpoint,
			boost::bind(&Getter::handle_connect, this,
			boost::asio::placeholders::error, ++endpoint_iterator));
	}
	else
	{
		error_output_ << "Error: " << err.message() << endl;
		Error();
	}
}

void Getter::handle_connect(const boost::system::error_code& err,
					tcp::resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		// The connection was successful. Send the request.
		boost::asio::async_write(socket_, request_,
			boost::bind(&Getter::handle_write_request, this,
			boost::asio::placeholders::error));
	}
	else if (endpoint_iterator != tcp::resolver::iterator())
	{
		// The connection failed. Try the next endpoint in the list.
		socket_.close();
		tcp::endpoint endpoint = *endpoint_iterator;
		socket_.async_connect(endpoint,
			boost::bind(&Getter::handle_connect, this,
			boost::asio::placeholders::error, ++endpoint_iterator));
	}
	else
	{
		error_output_ << "Error: " << err.message() << endl;
		Error();
	}
}

void Getter::handle_write_request(const boost::system::error_code& err)
{
	if (!err)
	{
		// Read the response status line.
		boost::asio::async_read_until(socket_, response_, "\r\n",
			boost::bind(&Getter::handle_read_status_line, this,
			boost::asio::placeholders::error));
	}
	else
	{
		error_output_ << "Error: " << err.message() << endl;
		Error();
	}
}

void Getter::handle_read_status_line(const boost::system::error_code& err)
{
	if (!err)
	{
		// Check that response is OK.
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			error_output_ << "Invalid response\n";
			Error();
			return;
		}
		if (status_code != 200)
		{
			error_output_ << "Response returned with status code ";
			error_output_ << status_code << endl;
			Error();
			return;
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
			boost::bind(&Getter::handle_read_headers, this,
			boost::asio::placeholders::error));
	}
	else
	{
		error_output_ << "Error: " << err << endl;
		Error();
	}
}

void Getter::handle_read_headers(const boost::system::error_code& err)
{
	if (!err)
	{
		// Process the response headers.
		std::istream response_stream(&response_);
		std::string header;
		while (std::getline(response_stream, header) && header != "\r")
		{
			//std::cout << header << "\n";
		}
		//std::cout << "\n";

		// Write whatever content we already have to output.
		if (response_.size() > 0)
			result_ << &response_;

		// Start reading remaining data until EOF.
		boost::asio::async_read(socket_, response_,
			boost::asio::transfer_at_least(1),
			boost::bind(&Getter::handle_read_content, this,
			boost::asio::placeholders::error));
	}
	else
	{
		error_output_ << "Error: " << err << endl;
		Error();
	}
}

void Getter::handle_read_content(const boost::system::error_code& err)
{
	if (!err)
	{
		// Write all of the data that has been read so far.
		result_ << &response_;

		// Continue reading remaining data until EOF.
		boost::asio::async_read(socket_, response_,
			boost::asio::transfer_at_least(1),
			boost::bind(&Getter::handle_read_content, this,
			boost::asio::placeholders::error));
	}
	else if (err != boost::asio::error::eof)
	{
		error_output_ << "Error: " << err << endl;
		Error();
	}
	else
		state = STATE_COMPLETE;
}

void Getter::Error()
{
	state = STATE_ERROR;
	io_service_.stop();
}

std::string Getter::GetResult() const
{
	if (state != STATE_COMPLETE)
	{
		return "";
	}
	
	return result_.str();
}

Getter::STATUS Getter::GetStatus()
{
	if (state == STATE_INWORK)
	{
		io_service_.poll();
	}
	
	return state;
}

Getter::Getter(const std::string& server, const std::string& path)
: resolver_(io_service_), socket_(io_service_), state(STATE_INWORK)
{
	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	std::ostream request_stream(&request_);
	request_stream << "GET " << path << " HTTP/1.0\r\n";
	request_stream << "Host: " << server << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	// Start an asynchronous resolve to translate the server and service names
	// into a list of endpoints.
	tcp::resolver::query query(server, "http");
	resolver_.async_resolve(query,
		boost::bind(&Getter::handle_resolve, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::iterator));
}

};

QT_TEST(httpget_test)
{
	std::stringstream resultstream, error_output;
	QT_CHECK(httpget::Get("vdrift.net", "/backend/vdrift.rdf", resultstream, std::cerr));
	
	httpget::Getter getter("vdrift.net", "/backend/vdrift.rdf");
	int ticks = 0;
	while (getter.GetStatus() != httpget::Getter::STATE_COMPLETE)
		ticks++;
	//std::cout << ticks << endl;
	QT_CHECK(ticks > 2);
	QT_CHECK(getter.GetStatus() == httpget::Getter::STATE_COMPLETE);
	QT_CHECK_EQUAL(resultstream.str(), getter.GetResult());
	
	//std::cout << "Errors:" << endl << error_output.str() << endl;
	//std::cout << "Data:" << endl << resultstream.str() << endl << endl;
}
