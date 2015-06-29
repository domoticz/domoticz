//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "stdafx.h"
#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

namespace http {
namespace server {

// this is the constructor for plain connections
connection::connection(boost::asio::io_service& io_service,
    connection_manager& manager, request_handler& handler, int timeout)
  : connection_manager_(manager),
    request_handler_(handler),
	timeout_(timeout),
	timer_(io_service, boost::posix_time::seconds( timeout ))
{
	secure_ = false;
	keepalive_ = false;
#ifdef NS_ENABLE_SSL
	sslsocket_ = NULL;
#endif
	socket_ = new boost::asio::ip::tcp::socket(io_service),
	m_lastresponse=mytime(NULL);
}

#ifdef NS_ENABLE_SSL
// this is the constructor for secure connections
connection::connection(boost::asio::io_service& io_service,
    connection_manager& manager, request_handler& handler, int timeout, boost::asio::ssl::context& context)
  : connection_manager_(manager),
    request_handler_(handler),
	timeout_(timeout),
	timer_(io_service, boost::posix_time::seconds( timeout ))
{
	secure_ = true;
	keepalive_ = false;
	socket_ = NULL;
	sslsocket_ = new ssl_socket(io_service, context);
	m_lastresponse=mytime(NULL);
}
#endif

#ifdef NS_ENABLE_SSL
// get the attached client socket of this connection
ssl_socket::lowest_layer_type& connection::socket()
{
  if (secure_) {
	return sslsocket_->lowest_layer();
  }
  else {
	return socket_->lowest_layer();
  }
}
#else
// alternative: get the attached client socket of this connection if ssl is not compiled in
boost::asio::ip::tcp::socket& connection::socket()
{
	return *socket_;
}
#endif

void connection::start()
{
    host_endpoint_ = socket().remote_endpoint().address().to_string();
    if (secure_) {
#ifdef NS_ENABLE_SSL
		// with ssl, we first need to complete the handshake before reading
  		sslsocket_->async_handshake(boost::asio::ssl::stream_base::server,
        	boost::bind(&connection::handle_handshake, shared_from_this(),
          	boost::asio::placeholders::error));
#endif
    }
    else {
		// start reading data
		read_more_plain();
    }
    m_lastresponse=mytime(NULL);
}

void connection::stop()
{
	socket().close();
}

void connection::handle_timeout(const boost::system::error_code& error)
{
		if (error != boost::asio::error::operation_aborted) {
			connection_manager_.stop(shared_from_this());
		}
}

#ifdef NS_ENABLE_SSL
void connection::handle_handshake(const boost::system::error_code& error)
{
    if (secure_) { // assert
		if (!error)
		{
			// handshake completed, start reading
			read_more_secure();
		}
		else
		{
			connection_manager_.stop(shared_from_this());
		}
  }
}

void connection::read_more_secure()
{
	// read chunks of max 4 KB
	boost::asio::streambuf::mutable_buffers_type buf = _buf.prepare(4096);

	// set timeout timer
	timer_.expires_from_now(boost::posix_time::seconds(timeout_));
	timer_.async_wait(boost::bind(&connection::handle_timeout, shared_from_this(), boost::asio::placeholders::error));

	// Perform read
	sslsocket_->async_read_some(buf,
		boost::bind(&connection::handle_read_secure, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void connection::handle_read_secure(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	// data read, no need for timeouts (RK, note: race condition)
	timer_.cancel();
    if (!error && bytes_transferred > 0)
    {
		// ensure written bytes in the buffer
		_buf.commit(bytes_transferred);
		m_lastresponse=mytime(NULL);
		boost::tribool result;
		/// The incoming request.
		request request_;
		const char *begin = boost::asio::buffer_cast<const char*>(_buf.data());
		try
		{
			request_parser_.reset();
			boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
				request_, begin, begin + _buf.size());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "Exception parsing http request.");
		}

		if (result) {
			size_t sizeread = begin - boost::asio::buffer_cast<const char*>(_buf.data());
			_buf.consume(sizeread);
			reply_.reset();
			const char *pConnection = request_.get_req_header(&request_, "Connection");
			keepalive_ = pConnection != NULL && boost::iequals(pConnection, "Keep-Alive");
			request_.keep_alive = keepalive_;
			request_handler_.handle_request(host_endpoint_, request_, reply_);
			boost::asio::async_write(*sslsocket_, reply_.to_buffers(),
				boost::bind(&connection::handle_write_secure, shared_from_this(),
				boost::asio::placeholders::error));
		}
		else if (!result)
		{
			keepalive_ = false;
			reply_ = reply::stock_reply(reply::bad_request);
			boost::asio::async_write(*sslsocket_, reply_.to_buffers(),
				boost::bind(&connection::handle_write_secure, shared_from_this(),
				boost::asio::placeholders::error));
		}
		else
		{
			read_more_secure();
		}
	}
    else if (error != boost::asio::error::operation_aborted)
    {
		connection_manager_.stop(shared_from_this());
    }
}

void connection::handle_write_secure(const boost::system::error_code& error)
{
	if (!error) {
		if (keepalive_) {
			// if a keep-alive connection is requested, we read the next request
			read_more_secure();
		}
		else {
			// Initiate graceful connection closure.
			boost::system::error_code ignored_ec;
			socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			connection_manager_.stop(shared_from_this());
		}
	}
	m_lastresponse=mytime(NULL);
}
#endif

void connection::read_more_plain()
{
	// read chunks of max 4 KB
	boost::asio::streambuf::mutable_buffers_type buf = _buf.prepare(4096);

	// set timeout timer
	timer_.expires_from_now(boost::posix_time::seconds(timeout_));
	timer_.async_wait(boost::bind(&connection::handle_timeout, shared_from_this(), boost::asio::placeholders::error));

	// Perform read
	socket_->async_read_some(buf,
		boost::bind(&connection::handle_read_plain, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void connection::handle_read_plain(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	// data read, no need for timeouts (RK, note: race condition)
	timer_.cancel();
    if (!error && bytes_transferred > 0)
    {
		// ensure written bytes in the buffer
		_buf.commit(bytes_transferred);
		m_lastresponse=mytime(NULL);
		boost::tribool result;
		/// The incoming request.
		request request_;
		const char *begin = boost::asio::buffer_cast<const char*>(_buf.data());
		try
		{
			request_parser_.reset();
			boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
				request_, begin, begin + _buf.size());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "Exception parsing http request.");
		}

		if (result) {
			size_t sizeread = begin - boost::asio::buffer_cast<const char*>(_buf.data());
			_buf.consume(sizeread);
			reply_.reset();
			const char *pConnection = request_.get_req_header(&request_, "Connection");
			keepalive_ = pConnection != NULL && boost::iequals(pConnection, "Keep-Alive");
			request_.keep_alive = keepalive_;
			request_handler_.handle_request(host_endpoint_, request_, reply_);
			boost::asio::async_write(*socket_, reply_.to_buffers(),
				boost::bind(&connection::handle_write_plain, shared_from_this(),
				boost::asio::placeholders::error));
		}
		else if (!result)
		{
			keepalive_ = false;
			reply_ = reply::stock_reply(reply::bad_request);
			boost::asio::async_write(*socket_, reply_.to_buffers(),
				boost::bind(&connection::handle_write_plain, shared_from_this(),
				boost::asio::placeholders::error));
		}
		else
		{
			read_more_plain();
		}
	}
    else if (error != boost::asio::error::operation_aborted)
    {
		connection_manager_.stop(shared_from_this());
    }
}

void connection::handle_write_plain(const boost::system::error_code& error)
{
	if (!error) {
		if (keepalive_) {
			// if a keep-alive connection is requested, we read the next request
			read_more_plain();
		}
		else {
			// Initiate graceful connection closure.
			boost::system::error_code ignored_ec;
			socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			connection_manager_.stop(shared_from_this());
		}
	}
	m_lastresponse=mytime(NULL);
}

connection::~connection()
{
	// free up resources, delete the socket pointers
	if (socket_) delete socket_;
#ifdef NS_ENABLE_SSL
	if (sslsocket_) delete sslsocket_;
#endif
}

} // namespace server
} // namespace http
