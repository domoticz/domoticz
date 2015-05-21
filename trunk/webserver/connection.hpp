//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"
#ifdef NS_ENABLE_SSL
#include <boost/asio/ssl.hpp>
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
#endif

namespace http {
namespace server {

class connection_manager;

/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
{
public:
  /// Construct a connection with the given io_service.
  explicit connection(boost::asio::io_service& io_service,
      connection_manager& manager, request_handler& handler, int timeout);
#ifdef NS_ENABLE_SSL
  explicit connection(boost::asio::io_service& io_service,
      connection_manager& manager, request_handler& handler, int timeout, boost::asio::ssl::context& context);
#endif
  ~connection();

  /// Get the socket associated with the connection.
#ifdef NS_ENABLE_SSL
  ssl_socket::lowest_layer_type& socket();
#else
  boost::asio::ip::tcp::socket& socket();
#endif

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

  /// Last user interaction
  time_t m_lastresponse;

  /// read timeout timer
  boost::asio::deadline_timer timer_;
  void handle_timeout(const boost::system::error_code& error);

private:
  /// Handle completion of a read operation.
  void handle_read_plain(const boost::system::error_code& e, std::size_t bytes_transferred);
  void read_more_plain();

  /// Handle completion of a write operation.
  void handle_write_plain(const boost::system::error_code& e);

#ifdef NS_ENABLE_SSL
  /// ssl handle functions
  void handle_read_secure(const boost::system::error_code& e, std::size_t bytes_transferred);
  void read_more_secure();
  void handle_write_secure(const boost::system::error_code& e);
#endif

  /// Socket for the (PLAIN) connection.
  boost::asio::ip::tcp::socket *socket_;
  //Host EndPoint
  std::string host_endpoint_;

  /// If this is a keep-alive connection or not
  bool keepalive_;

  /// timeouts (persistent and other) connections in seconds
  int timeout_;

  /// The manager for this connection.
  connection_manager& connection_manager_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;

  /// the buffer that we receive data in
  boost::asio::streambuf _buf;

  // secure connection members below
  // secure connection yes/no
  bool secure_;
#ifdef NS_ENABLE_SSL
  // the SSL socket
  ssl_socket *sslsocket_;
  void handle_handshake(const boost::system::error_code& error);
#endif
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
