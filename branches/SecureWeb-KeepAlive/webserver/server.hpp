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

namespace http {
namespace server {

/// The top-level class of the secure HTTP server.
class server
  : private boost::noncopyable
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      request_handler& user_request_handler, std::string certificatefile = "", std::string passphrase = "");

  /// Run the server's io_service loop.
  void run();

  /// Stop the server.
  void stop();

private:
  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& error);

  /// Handle a request to stop the server.
  void handle_stop();

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The handler for all incoming requests.
  request_handler& request_handler_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;

  connection_manager connection_manager_;
  /// determines if this is an SSL server or not
  bool secure_;

  /// read timeout in seconds
  int timeout_;

#ifdef NS_ENABLE_SSL
  /// The SSL context
  boost::asio::ssl::context context_;

  /// the certficiate passphrase
  std::string passphrase_;

  /// callback for passphrase
  std::string get_passphrase() const;
#endif

};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
