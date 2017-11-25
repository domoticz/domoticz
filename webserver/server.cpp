//
// server.cpp
// ~~~~~~~~~~
//
#include "stdafx.h"
#include <boost/bind.hpp>
#include "server.hpp"
#include <fstream>
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"

namespace http {
namespace server {


server_base::server_base(const server_settings & settings, request_handler & user_request_handler) :
		io_service_(),
		acceptor_(io_service_),
		settings_(settings),
		request_handler_(user_request_handler),
		timeout_(20), // default read timeout in seconds
		is_running(false),
		is_stop_complete(false) {
	if (!settings.is_enabled()) {
		throw std::invalid_argument("cannot initialize a disabled server (listening port cannot be empty or 0)");
	}
}

void server_base::init(init_connectionhandler_func init_connection_handler, accept_handler_func accept_handler) {

	init_connection_handler();

	if (!new_connection_) {
		throw std::invalid_argument("cannot initialize a server without a valid connection");
	}

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(settings_.listening_address, settings_.listening_port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	// bind to both ipv6 and ipv4 sockets for the "::" address only
	if (settings_.listening_address == "::") {
		acceptor_.set_option(boost::asio::ip::v6_only(false));
	}
	// bind to our port
	acceptor_.bind(endpoint);
	// listen for incoming requests
	acceptor_.listen();

	// start the accept thread
	acceptor_.async_accept(new_connection_->socket(), accept_handler);
}

void server_base::run() {
	// The io_service::run() call will block until all asynchronous operations
	// have finished. While the server is running, there is always at least one
	// asynchronous operation outstanding: the asynchronous accept call waiting
	// for new incoming connections.
	try {
		is_running = true;
		io_service_.run();
		is_running = false;
	} catch (std::exception& e) {
		_log.Log(LOG_ERROR, "[web:%s] exception occurred : '%s' (need to run again)", settings_.listening_port.c_str(), e.what());
		is_running = false;
		// Note: if acceptor is up everything is OK, we can call run() again
		//       but if the exception has broken the acceptor we cannot stop/start it and the next run() will exit immediatly.
		io_service_.reset(); // this call is needed before calling run() again
		throw;
	} catch (...) {
		_log.Log(LOG_ERROR, "[web:%s] unknown exception occurred (need to run again)", settings_.listening_port.c_str());
		is_running = false;
		// Note: if acceptor is up everything is OK, we can call run() again
		//       but if the exception has broken the acceptor we cannot stop/start it and the next run() will exit immediatly.
		io_service_.reset(); // this call is needed before calling run() again
		throw;
	}
}

/// Ask the server to stop using asynchronous command
void server_base::stop() {
	if (is_running) {
		// Post a call to the stop function so that server_base::stop() is safe to call from any thread.
		// Rene, set is_running to false, because the following is an io_service call, which makes is_running
		// never set to false whilst in the call itself
		is_running = false;
		io_service_.post(boost::bind(&server_base::handle_stop, this));
	} else {
		// if io_service is not running then the post call will not be performed
		handle_stop();
	}

	// Wait for acceptor and connections to stop
	int timeout = 15; // force stop after 15 seconds
	time_t start = mytime(NULL);
	while(true) {
		if (!is_running && is_stop_complete) {
			break;
		}
		if ((mytime(NULL) - start) > timeout) {
			// timeout occurred
			break;
		}
		sleep_milliseconds(500);
	}
	io_service_.stop();
}

void server_base::handle_stop() {
	try {
		boost::system::error_code ignored_ec;
		acceptor_.close(ignored_ec);
	} catch (...) {
		_log.Log(LOG_ERROR, "[web:%s] exception occurred while closing acceptor", settings_.listening_port.c_str());
	}
	connection_manager_.stop_all();
	is_stop_complete = true;
}

server::server(const server_settings & settings, request_handler & user_request_handler) :
		server_base(settings, user_request_handler) {
#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] create server using settings : %s", settings.listening_port.c_str(), settings.to_string().c_str());
#endif
	init(boost::bind(&server::init_connection, this),
			boost::bind(&server::handle_accept, this, _1));
}

void server::init_connection() {
	new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_, timeout_));
}

/**
 * accepting incoming requests and start the client connection loop
 */
void server::handle_accept(const boost::system::error_code& e) {
	if (!e) {
		connection_manager_.start(new_connection_);
		new_connection_.reset(new connection(io_service_,
				connection_manager_, request_handler_, timeout_));
		// listen for a subsequent request
		acceptor_.async_accept(new_connection_->socket(),
				boost::bind(&server::handle_accept, this,
						boost::asio::placeholders::error));
	}
}

#ifdef WWW_ENABLE_SSL
ssl_server::ssl_server(const ssl_server_settings & ssl_settings, request_handler & user_request_handler) :
		server_base(ssl_settings, user_request_handler),
		settings_(ssl_settings),
		context_(io_service_, ssl_settings.get_ssl_method())
{
#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] create ssl_server using ssl_server_settings : %s", ssl_settings.listening_port.c_str(), ssl_settings.to_string().c_str());
#endif
	init(boost::bind(&ssl_server::init_connection, this),
			boost::bind(&ssl_server::handle_accept, this, _1));
}

// this constructor will send std::bad_cast exception if the settings argument is not a ssl_server_settings object
ssl_server::ssl_server(const server_settings & settings, request_handler & user_request_handler) :
		server_base(settings, user_request_handler),
		settings_(dynamic_cast<ssl_server_settings const &>(settings)),
		context_(io_service_, dynamic_cast<ssl_server_settings const &>(settings).get_ssl_method()) {
#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] create ssl_server using server_settings : %s", settings.listening_port.c_str(), settings.to_string().c_str());
#endif
	init(boost::bind(&ssl_server::init_connection, this),
			boost::bind(&ssl_server::handle_accept, this, _1));
}

void ssl_server::init_connection() {

	new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_, timeout_, context_));

	// the following line gets the passphrase for protected private server keys
	context_.set_password_callback(boost::bind(&ssl_server::get_passphrase, this));

	if (settings_.options.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL options parameter !", settings_.listening_port.c_str());
	} else {
		context_.set_options(settings_.get_ssl_options());
	}
	char cipher_list[] = "ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS";
	SSL_CTX_set_cipher_list(context_.native_handle(), cipher_list);

	if (settings_.certificate_chain_file_path.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL certificate chain file parameter !", settings_.listening_port.c_str());
	} else {
		context_.use_certificate_chain_file(settings_.certificate_chain_file_path);
	}
	if (settings_.cert_file_path.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL certificate file parameter !", settings_.listening_port.c_str());
	} else {
		context_.use_certificate_file(settings_.cert_file_path, boost::asio::ssl::context::pem);
	}
	if (settings_.private_key_file_path.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL private key file parameter !", settings_.listening_port.c_str());
	} else {
		context_.use_private_key_file(settings_.private_key_file_path, boost::asio::ssl::context::pem);
	}

	// Do not work with mobile devices at this time (2016/02)
	if (settings_.verify_peer || settings_.verify_fail_if_no_peer_cert) {
		if (settings_.verify_file_path.empty()) {
			_log.Log(LOG_ERROR, "[web:%s] missing SSL verify file parameter !", settings_.listening_port.c_str());
		} else {
			context_.load_verify_file(settings_.verify_file_path);
			boost::asio::ssl::context::verify_mode verify_mode = 0;
			if (settings_.verify_peer) {
				verify_mode |= boost::asio::ssl::context::verify_peer;
			}
			if (settings_.verify_fail_if_no_peer_cert) {
				verify_mode |= boost::asio::ssl::context::verify_fail_if_no_peer_cert;
			}
			context_.set_verify_mode(verify_mode);
		}
	}
	// Load DH parameters
	if (settings_.tmp_dh_file_path.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL DH file parameter", settings_.listening_port.c_str());
	} else {
		std::ifstream ifs(settings_.tmp_dh_file_path.c_str());
		std::string content((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));
		if (content.find("BEGIN DH PARAMETERS") != std::string::npos) {
			context_.use_tmp_dh_file(settings_.tmp_dh_file_path);
#ifdef DEBUG_WWW
			_log.Log(LOG_STATUS, "[web:%s] 'BEGIN DH PARAMETERS' found in file %s", settings_.listening_port.c_str(), settings_.tmp_dh_file_path.c_str());
#endif
		} else {
			_log.Log(LOG_ERROR, "[web:%s] missing SSL DH parameters from file %s", settings_.listening_port.c_str(), settings_.tmp_dh_file_path.c_str());
		}
	}
}

/**
 * accepting incoming requests and start the client connection loop
 */
void ssl_server::handle_accept(const boost::system::error_code& e) {
	if (!e) {
		connection_manager_.start(new_connection_);
		new_connection_.reset(new connection(io_service_,
				connection_manager_, request_handler_, timeout_, context_));
		// listen for a subsequent request
		acceptor_.async_accept(new_connection_->socket(),
				boost::bind(&ssl_server::handle_accept, this,
						boost::asio::placeholders::error));
	}
}

std::string ssl_server::get_passphrase() const {
	return settings_.private_key_pass_phrase;
}
#endif

boost::shared_ptr<server_base> server_factory::create(const server_settings & settings, request_handler & user_request_handler) {
#ifdef WWW_ENABLE_SSL
		if (settings.is_secure()) {
			return create(dynamic_cast<ssl_server_settings const &>(settings), user_request_handler);
		}
#endif
		return boost::shared_ptr<server_base>(new server(settings, user_request_handler));
	}

#ifdef WWW_ENABLE_SSL
boost::shared_ptr<server_base> server_factory::create(const ssl_server_settings & ssl_settings, request_handler & user_request_handler) {
		return boost::shared_ptr<server_base>(new ssl_server(ssl_settings, user_request_handler));
	}
#endif

} // namespace server
} // namespace http
