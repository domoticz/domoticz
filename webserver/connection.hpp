//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
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
#ifdef WWW_ENABLE_SSL
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
#ifdef WWW_ENABLE_SSL
  explicit connection(boost::asio::io_service& io_service,
      connection_manager& manager, request_handler& handler, int timeout, boost::asio::ssl::context& context);
#endif
  ~connection();

  /// Get the socket associated with the connection.
#ifdef WWW_ENABLE_SSL
  ssl_socket::lowest_layer_type& socket();
#else
  boost::asio::ip::tcp::socket& socket();
#endif

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

  /// Timer handlers
  void handle_read_timeout(const boost::system::error_code& error);
  void handle_abandoned_timeout(const boost::system::error_code& error);

private:
  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);
  void read_more();

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e);

	/// Initialize read timeout timer
	void set_read_timeout();
	/// Stop read timeout timer
	void cancel_read_timeout();
	/// Reset read timeout timer
	void reset_read_timeout();

	/// Schedule abandoned timeout timer
	void set_abandoned_timeout();
	/// Stop abandoned timeout timer
	void cancel_abandoned_timeout();
	/// Reschedule abandoned timeout timer
	void reset_abandoned_timeout();

  /// Socket for the (PLAIN) connection.
  boost::asio::ip::tcp::socket *socket_;
  //Host EndPoint
  std::string host_endpoint_address_;
  std::string host_endpoint_port_;

  /// If this is a keep-alive connection or not
  bool keepalive_;

  /// Read timeout in seconds
  int read_timeout_;

  /// Read timeout timer
  boost::asio::deadline_timer read_timer_;

  /// Abandoned connection timeout (in seconds)
  long default_abandoned_timeout_;
  /// Abandoned timeout timer
  boost::asio::deadline_timer abandoned_timer_;

  /// The manager for this connection.
  connection_manager& connection_manager_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;

  /// The buffer that we receive data in
  boost::asio::streambuf _buf;

  /// The status of the connection (can be initializing, handshaking, waiting, reading, writing)
  enum connection_status {
    INITIALIZING,
    WAITING_HANDSHAKE,
	ENDING_HANDSHAKE,
    WAITING_READ,
	READING,
    WAITING_WRITE,
	ENDING_WRITE
  } status_;

  /// The default number of request to handle with the connection when keep-alive is enabled
  unsigned int default_max_requests_;

  // secure connection members below
  // secure connection yes/no
  bool secure_;
#ifdef WWW_ENABLE_SSL
  // the SSL socket
  ssl_socket *sslsocket_;
  void handle_handshake(const boost::system::error_code& error);
#endif
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
