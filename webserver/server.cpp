//
// server.cpp
// ~~~~~~~~~~
//
#include "stdafx.h"
#include <boost/bind.hpp>
#include "server.hpp"
#include <fstream>
#include "../main/Logger.h"

namespace http {
namespace server {


server_base::server_base(const server_settings & settings, request_handler & user_request_handler) :
		io_service_(),
		acceptor_(io_service_),
		settings_(settings),
		request_handler_(user_request_handler),
		timeout_(20) { // default read timeout in seconds
	//_log.Log(LOG_STATUS, "[web:%s] create server_base using settings : %s", settings.listening_port.c_str(), settings.to_string().c_str());
	if (!settings.is_enabled()) {
		throw std::invalid_argument("cannot initialize a disabled server (listening port cannot be empty or 0)");
	}
}

void server_base::init() {
	init_connection();

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(settings_.listening_address, settings_.listening_port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	// bind to our port
	acceptor_.bind(endpoint);
	// listen for incoming requests
	acceptor_.listen();
	// start the accept thread
	acceptor_.async_accept(new_connection_->socket(),
			boost::bind(&server_base::handle_accept, this,
					boost::asio::placeholders::error));
}

void server_base::run() {
	// The io_service::run() call will block until all asynchronous operations
	// have finished. While the server is running, there is always at least one
	// asynchronous operation outstanding: the asynchronous accept call waiting
	// for new incoming connections.
	try {
		io_service_.run();
	} catch (std::exception& e) {
		_log.Log(LOG_ERROR, "[web:%s] exception occurred : '%s' (need to run again)", settings_.listening_port.c_str(), e.what());
		handle_stop(); // dispatch or post call does NOT work because it is pushed in the event queue (executed only on next io service run)
		io_service_.reset(); // this call is needed before calling run() again
		throw e;
	}
	catch (...) {
		_log.Log(LOG_ERROR, "[web:%s] unknown exception occurred (need to run again)", settings_.listening_port.c_str());
		handle_stop(); // dispatch or post call does NOT work because it is pushed in the event queue (executed only on next io service run)
		io_service_.reset(); // this call is needed before calling run() again
		throw;
	}
}

void server_base::stop() {
	// Post a call to the stop function so that server_base::stop() is safe to call
	// from any thread.
	io_service_.post(boost::bind(&server_base::handle_stop, this));
}

void server_base::handle_stop() {
	// The server is stopped by cancelling all outstanding asynchronous
	// operations. Once all operations have finished the io_service::run() call
	// will exit.
	acceptor_.close();
	connection_manager_.stop_all();
}

server::server(const server_settings & settings, request_handler & user_request_handler) :
		server_base(settings, user_request_handler) {
	//_log.Log(LOG_STATUS, "[web:%s] create server using settings : %s", settings.listening_port.c_str(), settings.to_string().c_str());
	init();
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

#ifdef NS_ENABLE_SSL
ssl_server::ssl_server(const ssl_server_settings & ssl_settings, request_handler & user_request_handler) :
		server_base(ssl_settings, user_request_handler),
		settings_(ssl_settings),
		context_(io_service_, ssl_settings.get_ssl_method())
{
	//_log.Log(LOG_STATUS, "[web:%s] create ssl_server using ssl_server_settings : %s", ssl_settings.listening_port.c_str(), ssl_settings.to_string().c_str());
	init();
}

// this constructor will send std::bad_cast exception if the settings argument is not a ssl_server_settings object
ssl_server::ssl_server(const server_settings & settings, request_handler & user_request_handler) :
		server_base(settings, user_request_handler),
		settings_(dynamic_cast<ssl_server_settings const &>(settings)),
		context_(io_service_, dynamic_cast<ssl_server_settings const &>(settings).get_ssl_method()) {
	//_log.Log(LOG_STATUS, "[web:%s] create ssl_server using server_settings : %s", settings.listening_port.c_str(), settings.to_string().c_str());
	init();
}

void ssl_server::init_connection() {
	//_log.Log(LOG_STATUS, "[web:%s] ssl_server::init_connection() : new connection using settings : %s", settings_.listening_port.c_str(), settings_.to_string().c_str());

	new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_, timeout_, context_));

	// the following line gets the passphrase for protected private server keys
	context_.set_password_callback(boost::bind(&ssl_server::get_passphrase, this));

	if (settings_.options.empty()) {
		_log.Log(LOG_ERROR, "[web:%s] missing SSL options parameter !", settings_.listening_port.c_str());
	} else {
		context_.set_options(settings_.get_ssl_options());
	}

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
			boost::asio::ssl::context::verify_mode verify_mode;
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
			//_log.Log(LOG_STATUS, "[web:%s] 'BEGIN DH PARAMETERS' found in file %s", settings_.listening_port.c_str(), settings_.tmp_dh_file_path.c_str());
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

server_base * server_factory::create(const server_settings & settings, request_handler & user_request_handler) {
#ifdef NS_ENABLE_SSL
		if (settings.is_secure()) {
			return create(dynamic_cast<ssl_server_settings const &>(settings), user_request_handler);
		}
#endif
		return new server(settings, user_request_handler);
	}

#ifdef NS_ENABLE_SSL
server_base * server_factory::create(const ssl_server_settings & ssl_settings, request_handler & user_request_handler) {
		return new ssl_server(ssl_settings, user_request_handler);
	}
#endif

} // namespace server
} // namespace http
