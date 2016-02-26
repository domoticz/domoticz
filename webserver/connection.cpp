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
#include <boost/thread/mutex.hpp>
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
	timer_(io_service, boost::posix_time::seconds( timeout )),
	websocket_handler(this, request_handler_.Get_myWebem()),
	default_abandoned_timeout_(20 * 60), // 20mn before stopping abandoned connection
	abandoned_timer_(io_service, boost::posix_time::seconds(default_abandoned_timeout_))
{
	secure_ = false;
	keepalive_ = false;
	write_in_progress = false;
	connection_type = connection_http;
#ifdef WWW_ENABLE_SSL
	sslsocket_ = NULL;
#endif
	socket_ = new boost::asio::ip::tcp::socket(io_service);
}

#ifdef WWW_ENABLE_SSL
// this is the constructor for secure connections
connection::connection(boost::asio::io_service& io_service,
    connection_manager& manager, request_handler& handler, int timeout, boost::asio::ssl::context& context)
  : connection_manager_(manager),
    request_handler_(handler),
	timeout_(timeout),
	timer_(io_service, boost::posix_time::seconds( timeout )),
	websocket_handler(this, request_handler_.Get_myWebem()),
	default_abandoned_timeout_(20*60), // 20mn before stopping abandoned connection
	abandoned_timer_(io_service, boost::posix_time::seconds(default_abandoned_timeout_))
{
	secure_ = true;
	keepalive_ = false;
	write_in_progress = false;
	connection_type = connection_http;
	socket_ = NULL;
	sslsocket_ = new ssl_socket(io_service, context);
}
#endif

#ifdef WWW_ENABLE_SSL
// get the attached client socket of this connection
ssl_socket::lowest_layer_type& connection::socket()
{
  if (secure_) {
	return sslsocket_->lowest_layer();
  } else {
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
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint endpoint = socket().remote_endpoint(ec);
	if (ec) {
		// Prevent the exception to be thrown to run to avoid the server to be locked (still listening but no more connection or stop).
		// If the exception returns to WebServer to also create a exception loop.
		_log.Log(LOG_ERROR,"Getting error '%s' while getting remote_endpoint in connection::start", ec.message().c_str());
		connection_manager_.stop(shared_from_this());
		return;
	}

	set_abandoned_timeout();
	host_endpoint_ = endpoint.address().to_string();
	if (secure_) {
#ifdef WWW_ENABLE_SSL
		// with ssl, we first need to complete the handshake before reading
		sslsocket_->async_handshake(boost::asio::ssl::stream_base::server,
			boost::bind(&connection::handle_handshake, shared_from_this(),
			boost::asio::placeholders::error));
#endif
	}
	else {
		// start reading data
		read_more();
	}
}

void connection::stop()
{
	switch (connection_type) {
	case connection_websocket:
		// todo: send close frame and wait for writeQ to flush
		websocket_handler.SendClose("");
		break;
	case connection_closing:
		// todo: wait for writeQ to flush, so client can receive the close frame
		break;
	}
	// Cancel timers
	cancel_abandoned_timeout();
	timer_.cancel();

	// Initiate graceful connection closure.
	boost::system::error_code ignored_ec;
	socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec); // @note For portable behaviour with respect to graceful closure of a
																				// connected socket, call shutdown() before closing the socket.
	socket().close();
}

void connection::handle_timeout(const boost::system::error_code& error)
{
		if (error != boost::asio::error::operation_aborted) {
			switch (connection_type) {
			case connection_http:
				connection_manager_.stop(shared_from_this());
				break;
			case connection_websocket:
				websocket_handler.SendPing();
				break;
			}
		}
}

#ifdef WWW_ENABLE_SSL
void connection::handle_handshake(const boost::system::error_code& error)
{
    if (secure_) { // assert
		if (!error)
		{
			// handshake completed, start reading
			read_more();
		}
		else
		{
			connection_manager_.stop(shared_from_this());
		}
  }
}
#endif

void connection::read_more()
{
	// read chunks of max 4 KB
	boost::asio::streambuf::mutable_buffers_type buf = _buf.prepare(4096);

	// set timeout timer
	timer_.expires_from_now(boost::posix_time::seconds(timeout_));
	timer_.async_wait(boost::bind(&connection::handle_timeout, shared_from_this(), boost::asio::placeholders::error));

	if (secure_) {
#ifdef WWW_ENABLE_SSL
		// Perform secure read
		sslsocket_->async_read_some(buf,
			boost::bind(&connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
#endif
	}
	else {
		// Perform plain read
		socket_->async_read_some(buf,
			boost::bind(&connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void connection::SocketWrite(const std::string &buf)
{
	// do not call directly, use MyWrite()
	if (write_in_progress) {
		// something went wrong, this shouldnt happen
	}
	write_in_progress = true;
	write_buffer = buf;
	if (secure_) {
#ifdef WWW_ENABLE_SSL
		boost::asio::async_write(*sslsocket_, boost::asio::buffer(write_buffer), boost::bind(&connection::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#endif
	}
	else {
		boost::asio::async_write(*socket_, boost::asio::buffer(write_buffer), boost::bind(&connection::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

}

void connection::MyWrite(const std::string &buf)
{
	switch (connection_type) {
	case connection_http:
	case connection_websocket:
		// we dont send data anymore in websocket closing state
		boost::unique_lock<boost::mutex>(writeMutex);
		if (write_in_progress) {
			// write in progress, add to queue
			writeQ.push(buf);
		}
		else {
			SocketWrite(buf);
		}
		break;
	}
}

void connection::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	// data read, no need for timeouts (RK, note: race condition)
	timer_.cancel();
    if (!error && bytes_transferred > 0)
    {
		// ensure written bytes in the buffer
		_buf.commit(bytes_transferred);
		boost::tribool result;

		// http variables
		/// The incoming request.
		request request_;
		/// our response
		reply reply_;
		const char *begin;
		// websocket variables
		size_t bytes_consumed;

		switch (connection_type) {
		case connection_http:
			begin = boost::asio::buffer_cast<const char*>(_buf.data());
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
				request_.host = host_endpoint_;
				if (request_.host.substr(0, 7) == "::ffff:") {
					request_.host = request_.host.substr(7);
				}
				request_handler_.handle_request(request_, reply_);
				MyWrite(reply_.to_string(request_.method));
				if (reply_.status == reply::switching_protocols) {
					// this was an upgrade request
					connection_type = connection_websocket;
					// from now on we are a persistant connection
					keepalive_ = true;
					// keep sessionid to access our session during websockets requests
					websocket_handler.store_session_id(request_, reply_);
					// todo: check if multiple connection from the same client in CONNECTING state?
				}
				if (keepalive_) {
					read_more();
				}
			}
			else if (!result)
			{
				keepalive_ = false;
				reply_ = reply::stock_reply(reply::bad_request);
				MyWrite(reply_.to_string(request_.method));
				if (keepalive_) {
					read_more();
				}
			}
			else
			{
				read_more();
			}
			break;
		case connection_websocket:
		case connection_closing:
			begin = boost::asio::buffer_cast<const char*>(_buf.data());
			result = websocket_handler.parse((const unsigned char *)begin, _buf.size(), bytes_consumed, keepalive_);
			_buf.consume(bytes_consumed);
			if (result) {
				// we received a complete packet (that was handled already)
				if (keepalive_) {
					read_more();
				}
				else {
					// a connection close control packet was received
					// todo: wait for writeQ to flush?
					connection_type = connection_closing;
				}
			}
			else if (!result) {
				// we received a complete frame but not a complete packet yet or a control frame)
				read_more();
			}
			else {
				// we received an incomplete frame
				read_more();
			}
			break;
		}

	}
    else if (error != boost::asio::error::operation_aborted)
    {
		connection_manager_.stop(shared_from_this());
    }
}

void connection::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
{
	boost::unique_lock<boost::mutex>(writeMutex);
	write_in_progress = false;
	if (!error) {
		if (!writeQ.empty()) {
			SocketWrite(writeQ.front());
			writeQ.pop();
		}
		else if (!keepalive_) {
			connection_manager_.stop(shared_from_this());
		}
	}
}

connection::~connection()
{
	// free up resources, delete the socket pointers
	if (socket_) delete socket_;
#ifdef WWW_ENABLE_SSL
	if (sslsocket_) delete sslsocket_;
#endif
}

/// schedule abandoned timeout timer
void connection::set_abandoned_timeout() {
	abandoned_timer_.expires_from_now(boost::posix_time::seconds(default_abandoned_timeout_));
	abandoned_timer_.async_wait(boost::bind(&connection::handle_abandoned_timeout, shared_from_this(), boost::asio::placeholders::error));
}

/// simply cancel abandoned timeout timer
void connection::cancel_abandoned_timeout() {
	abandoned_timer_.cancel();
}

/// reschedule abandoned timeout timer
void connection::reset_abandoned_timeout() {
	cancel_abandoned_timeout();
	set_abandoned_timeout();
}

/// stop connection on abandoned timeout
void connection::handle_abandoned_timeout(const boost::system::error_code& error) {
	if (error != boost::asio::error::operation_aborted) {
		_log.Log(LOG_ERROR, "%s -> connection abandoned", host_endpoint_.c_str());
		connection_manager_.stop(shared_from_this());
	}
}

} // namespace server
} // namespace http
