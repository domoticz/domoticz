//
// server.hpp
// ~~~~~~~~~~
//

#ifndef HTTP_SSLSERVER_HPP
#define HTTP_SSLSERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "server_settings.hpp"

namespace http {
namespace server {

/// The top-level class of the HTTP(S) server.
class server_base : private boost::noncopyable {
public:
	/// Construct the server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	explicit server_base(const server_settings & settings, request_handler & user_request_handler);
	virtual ~server_base() {}

	/// Run the server's io_service loop.
	void run();

	/// Stop the server.
	void stop();

	/// Print server settings to string (debug purpose)
	virtual std::string to_string() const = 0;
protected:
	void init();

	/// Initialize acceptor
	virtual void init_connection() =0;

	/// Handle completion of an asynchronous accept operation.
	virtual void handle_accept(const boost::system::error_code& error) = 0;

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// The handler for all incoming requests.
	request_handler& request_handler_;

	/// The next connection to be accepted.
	connection_ptr new_connection_;

	connection_manager connection_manager_;
	/// server settings
	server_settings settings_;

	/// read timeout in seconds
	int timeout_;
private:
	/// Handle a request to stop the server.
	void handle_stop();
};

class server : public server_base {
public:
	/// Construct the HTTP server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	server(const server_settings & settings, request_handler & user_request_handler);
	virtual ~server() {}

	/// Print server settings to string (debug purpose)
	virtual std::string to_string() const {
		return "'server[" + settings_.to_string() + "]'";
	}
protected:
	/// Initialize acceptor
	virtual void init_connection();

	/// Handle completion of an asynchronous accept operation.
	virtual void handle_accept(const boost::system::error_code& error);
};

#ifdef NS_ENABLE_SSL
class ssl_server : public server_base {
public:
	/// Construct the HTTPS server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	ssl_server(const ssl_server_settings & ssl_settings, request_handler & user_request_handler);
	ssl_server(const server_settings & settings, request_handler & user_request_handler);
	virtual ~ssl_server() {}

	/// Print server settings to string (debug purpose)
	virtual std::string to_string() const {
		return "'ssl_server[" + settings_.to_string() + "]'";
	}

protected:
	/// Initialize acceptor
	virtual void init_connection();

	/// Handle completion of an asynchronous accept operation.
	virtual void handle_accept(const boost::system::error_code& error);

	// The HTTPS server settings
	ssl_server_settings settings_;

private:
	/// callback for the certficiate passphrase
	std::string get_passphrase() const;

	/// The SSL context
	boost::asio::ssl::context context_;
};
#endif

/// server factory
class server_factory {
public:
	static server_base * create(const server_settings & settings, request_handler & user_request_handler);

#ifdef NS_ENABLE_SSL
	static server_base * create(const ssl_server_settings & ssl_settings, request_handler & user_request_handler);
#endif
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
