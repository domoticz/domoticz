//
// server.cpp
// ~~~~~~~~~~
//
#include "stdafx.h"
#include <boost/bind.hpp>
#include "server.hpp"
#include <fstream>

namespace http {
namespace server {

server::server(const std::string& address, const std::string& port,
    request_handler& user_request_handler, std::string certificatefile, std::string passphrase)
  : io_service_(),
    acceptor_(io_service_), 
#ifdef NS_ENABLE_SSL
	passphrase_(passphrase),
    context_(io_service_, boost::asio::ssl::context::sslv23),
#endif
    request_handler_( user_request_handler),
	timeout_(20), // default read timeout in seconds
	secure_(false)
{
#ifdef NS_ENABLE_SSL
	secure_ = (certificatefile != "");
#endif
  if (!secure_)
  {
	  new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_, timeout_));
  }
  else
  {
#ifdef NS_ENABLE_SSL
	  new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_, timeout_,context_));
#endif
  }
  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(io_service_);
  boost::asio::ip::tcp::resolver::query query(address, port);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  if (secure_) {
#ifdef NS_ENABLE_SSL
	// the following line gets the passphrase for protected private server keys
	context_.set_password_callback(boost::bind(&server::get_passphrase, this));
	context_.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::single_dh_use);
	context_.use_certificate_chain_file(certificatefile);
	context_.use_private_key_file(certificatefile, boost::asio::ssl::context::pem);

	//Check if certificate contains DH parameters
	std::ifstream ifs(certificatefile.c_str());
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	if (content.find("BEGIN DH PARAMETERS")!=std::string::npos)
		context_.use_tmp_dh_file(certificatefile);
#endif
  }
  // bind to our port
  acceptor_.bind(endpoint);
  // listen for incoming requests
  acceptor_.listen();
  // start the accept thread
  acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&server::handle_accept, this,
        boost::asio::placeholders::error));
}

void server::run()
{
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  io_service_.run();
}

void server::stop()
{
  // Post a call to the stop function so that server::stop() is safe to call
  // from any thread.
  io_service_.post(boost::bind(&server::handle_stop, this));
}

// accepting incoming requests and start the client connection loop
void server::handle_accept(const boost::system::error_code& e)
{
  if (!e)
  {
    connection_manager_.start(new_connection_);
#ifdef NS_ENABLE_SSL
    if (secure_) {
    	new_connection_.reset(new connection(io_service_,
          connection_manager_, request_handler_, timeout_, context_));
    }
    else {
    	new_connection_.reset(new connection(io_service_,
          connection_manager_, request_handler_, timeout_));
    }
#else
    new_connection_.reset(new connection(io_service_,
          connection_manager_, request_handler_, timeout_));
#endif
	// listen for a subsequent request
    acceptor_.async_accept(new_connection_->socket(),
        boost::bind(&server::handle_accept, this,
          boost::asio::placeholders::error));
  }
}

#ifdef NS_ENABLE_SSL
std::string server::get_passphrase() const
{
	return passphrase_;
}
#endif

void server::handle_stop()
{
  // The server is stopped by cancelling all outstanding asynchronous
  // operations. Once all operations have finished the io_service::run() call
  // will exit.
  acceptor_.close();
  connection_manager_.stop_all();
}

} // namespace server
} // namespace http
