//
// server.hpp
// ~~~~~~~~~~
//
#pragma once
#ifndef HTTP_SSLSERVER_HPP
#define HTTP_SSLSERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include "../main/Noncopyable.h"
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "server_settings.hpp"

namespace http {
namespace server {

typedef boost::function< void() > init_connectionhandler_func;
typedef boost::function< void(const boost::system::error_code & error) > accept_handler_func;

/// The top-level class of the HTTP(S) server.
class server_base : private domoticz::noncopyable {
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
	virtual std::string to_string() const {
		return "'server_base[" + settings_.to_string() + "]'";
	}
protected:
	void init(init_connectionhandler_func init_connection_handler, accept_handler_func accept_handler);

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

	/// indicate if the server is running
	bool is_running;

	/// indicate if the server is stopped (acceptor and connections)
	bool is_stop_complete;

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
	virtual std::string to_string() const override {
		return "'server[" + settings_.to_string() + "]'";
	}
protected:
	/// Initialize acceptor
	void init_connection();

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& error);
private:
};

#ifdef WWW_ENABLE_SSL
class ssl_server : public server_base {
public:
	/// Construct the HTTPS server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	ssl_server(const ssl_server_settings & ssl_settings, request_handler & user_request_handler);
	ssl_server(const server_settings & settings, request_handler & user_request_handler);
	virtual ~ssl_server() {}

	/// Print server settings to string (debug purpose)
	virtual std::string to_string() const override {
		return "'ssl_server[" + settings_.to_string() + "]'";
	}

protected:
	/// Initialize acceptor
	void init_connection();

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(const boost::system::error_code& error);

	// The HTTPS server settings
	ssl_server_settings settings_;

private:
	/// Reload certificate and SSL params if they're changed
	void reinit_connection();
	time_t dhparam_tm_;
	time_t cert_tm_;
	time_t cert_chain_tm_;

	/// callback for the certficiate passphrase
	std::string get_passphrase() const;

	/// The SSL context
	boost::asio::ssl::context context_;
};
#endif

/// server factory
class server_factory {
public:
	static std::shared_ptr<server_base> create(const server_settings & settings, request_handler & user_request_handler);

#ifdef WWW_ENABLE_SSL
	static std::shared_ptr<server_base> create(const ssl_server_settings & ssl_settings, request_handler & user_request_handler);
#endif
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
