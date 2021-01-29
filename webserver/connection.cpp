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
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "mime_types.hpp"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

using namespace boost::placeholders;

namespace http {
	namespace server {
		extern std::string convert_to_http_date(time_t time);
		extern time_t last_write_time(const std::string& path);

		// this is the constructor for plain connections
		connection::connection(boost::asio::io_service &io_service, connection_manager &manager, request_handler &handler, int read_timeout)
			: send_buffer_(nullptr)
			, read_timeout_(read_timeout)
			, read_timer_(io_service, boost::posix_time::seconds(read_timeout))
			, default_abandoned_timeout_(20 * 60)
			// 20mn before stopping abandoned connection
			, abandoned_timer_(io_service, boost::posix_time::seconds(default_abandoned_timeout_))
			, connection_manager_(manager)
			, request_handler_(handler)
			, status_(INITIALIZING)
			, default_max_requests_(20)
			, websocket_parser(boost::bind(&connection::MyWrite, this, _1), handler.Get_myWebem(), boost::bind(&connection::WS_Write, this, _1))
		{
			secure_ = false;
			keepalive_ = false;
			write_in_progress = false;
			connection_type = ConnectionType::connection_http;
#ifdef WWW_ENABLE_SSL
			sslsocket_ = nullptr;
#endif
			socket_ = new boost::asio::ip::tcp::socket(io_service);
		}

#ifdef WWW_ENABLE_SSL
		// this is the constructor for secure connections
		connection::connection(boost::asio::io_service &io_service, connection_manager &manager, request_handler &handler, int read_timeout, boost::asio::ssl::context &context)
			: send_buffer_(nullptr)
			, read_timeout_(read_timeout)
			, read_timer_(io_service, boost::posix_time::seconds(read_timeout))
			, default_abandoned_timeout_(20 * 60)
			// 20mn before stopping abandoned connection
			, abandoned_timer_(io_service, boost::posix_time::seconds(default_abandoned_timeout_))
			, connection_manager_(manager)
			, request_handler_(handler)
			, status_(INITIALIZING)
			, default_max_requests_(20)
			, websocket_parser(boost::bind(&connection::MyWrite, this, _1), handler.Get_myWebem(), boost::bind(&connection::WS_Write, this, _1))
		{
			secure_ = true;
			keepalive_ = false;
			write_in_progress = false;
			connection_type = ConnectionType::connection_http;
			socket_ = nullptr;
			sslsocket_ = new ssl_socket(io_service, context);
		}
#endif

#ifdef WWW_ENABLE_SSL
		// get the attached client socket of this connection
		ssl_socket::lowest_layer_type& connection::socket()
		{
			if (secure_) {
				return sslsocket_->lowest_layer();
			}
			return socket_->lowest_layer();
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
				_log.Log(LOG_ERROR, "Getting error '%s' while getting remote_endpoint in connection::start", ec.message().c_str());
				connection_manager_.stop(shared_from_this());
				return;
			}
			host_endpoint_address_ = endpoint.address().to_string();
			//std::stringstream sstr;
			//sstr << endpoint.port();
			//sstr >> host_endpoint_port_;

			set_abandoned_timeout();

			if (secure_) {
#ifdef WWW_ENABLE_SSL
				status_ = WAITING_HANDSHAKE;
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
			case ConnectionType::connection_websocket:
				// todo: send close frame and wait for writeQ to flush
				//websocket_parser.SendClose("");
				websocket_parser.Stop();
				break;
			case ConnectionType::connection_websocket_closing:
				// todo: wait for writeQ to flush, so client can receive the close frame
				websocket_parser.Stop();
				break;
			}
			// Cancel timers
			cancel_abandoned_timeout();
			cancel_read_timeout();

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
				case ConnectionType::connection_http:
					// Timers should be cancelled before stopping to remove tasks from the io_service.
					// The io_service will stop naturally when every tasks are removed.
					// If timers are not cancelled, the exception ERROR_ABANDONED_WAIT_0 is thrown up to the io_service::run() caller.
					cancel_abandoned_timeout();
					cancel_read_timeout();

					try {
						// Initiate graceful connection closure.
						boost::system::error_code ignored_ec;
						socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec); // @note For portable behaviour with respect to graceful closure of a
																									// connected socket, call shutdown() before closing the socket.
						socket().close(ignored_ec);
					}
					catch (...) {
						_log.Log(LOG_ERROR, "%s -> exception thrown while stopping connection", host_endpoint_address_.c_str());
					}
					break;
				case ConnectionType::connection_websocket:
					websocket_parser.SendPing();
					break;
				}
			}
		}

#ifdef WWW_ENABLE_SSL
		void connection::handle_handshake(const boost::system::error_code& error)
		{
			status_ = ENDING_HANDSHAKE;
			if (secure_) { // assert
				if (!error)
				{
					// handshake completed, start reading
					read_more();
				}
				else
				{
					// _log.Log(LOG_ERROR, "connection::handle_handshake Error: %s", error.message().c_str());
					connection_manager_.stop(shared_from_this());
				}
			}
		}
#endif

		void connection::read_more()
		{
			status_ = WAITING_READ;

			// read chunks of max 4 KB
			boost::asio::streambuf::mutable_buffers_type buf = _buf.prepare(4096);

			// set timeout timer
			reset_read_timeout();

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

		void connection::SocketWrite(const std::string& buf)
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

		void connection::WS_Write(const std::string& resp)
		{
			if (connection_type == ConnectionType::connection_websocket) {
				MyWrite(CWebsocketFrame::Create(opcode_text, resp, false));
			}
			else {
				// socket connection not set up yet, add to queue
				std::unique_lock<std::mutex> lock(writeMutex);
				writeQ.push_back(CWebsocketFrame::Create(opcode_text, resp, false));
			}
		}

		void connection::MyWrite(const std::string& buf)
		{
			switch (connection_type) {
			case ConnectionType::connection_http:
			case ConnectionType::connection_websocket:
				// we dont send data anymore in websocket closing state
				std::unique_lock<std::mutex> lock(writeMutex);
				if (write_in_progress) {
					// write in progress, add to queue
					writeQ.push_back(buf);
				}
				else {
					SocketWrite(buf);
				}
				break;
			}
		}

		void connection::handle_write_file(const boost::system::error_code& error, size_t bytes_transferred)
		{
			if (!error && sendfile_.is_open() && !sendfile_.eof())
			{
#define FILE_SEND_BUFFER_SIZE 16*1024
				if (!send_buffer_)
					send_buffer_ = new uint8_t[FILE_SEND_BUFFER_SIZE];
				size_t bread = static_cast<size_t>(sendfile_.read((char*)send_buffer_, FILE_SEND_BUFFER_SIZE).gcount());
				if (bread <= 0)
				{
					//Error reading file!
					return;
				};
				if (secure_) {
#ifdef WWW_ENABLE_SSL
					boost::asio::async_write(*sslsocket_, boost::asio::buffer(send_buffer_, bread), boost::bind(&connection::handle_write_file, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#endif
				}
				else {
					boost::asio::async_write(*socket_, boost::asio::buffer(send_buffer_, bread), boost::bind(&connection::handle_write_file, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
				}
				return;
			}

			if (sendfile_.is_open())
				sendfile_.close();

			delete[] send_buffer_;
			send_buffer_ = nullptr;
			connection_manager_.stop(shared_from_this());
		}

		bool connection::send_file(const std::string& filename, std::string& attachment_name, reply& rep)
		{
			boost::system::error_code write_error;

			rep = reply::stock_reply(reply::ok);

			sendfile_.open(filename.c_str(), std::ios::in | std::ios::binary); //we open this file
			if (!sendfile_.is_open())
			{
				//File not found!
				rep = reply::stock_reply(reply::not_found);
				return false;
			}
			time_t ftime = last_write_time(filename);

			sendfile_.seekg(0, std::ios::end);
			std::streamsize total_size = sendfile_.tellg();
			sendfile_.seekg(0, std::ios::beg);

			reply::add_header(&rep, "Cache-Control", "max-age=0, private");
			reply::add_header(&rep, "Accept-Ranges", "bytes");
			reply::add_header(&rep, "Date", convert_to_http_date(time(nullptr)));
			reply::add_header(&rep, "Last-Modified", convert_to_http_date(ftime));
			reply::add_header(&rep, "Server", "Apache/2.2.22");

			std::size_t last_dot_pos = filename.find_last_of('.');
			if (last_dot_pos != std::string::npos) {
				std::string file_extension = filename.substr(last_dot_pos + 1);
				std::string mime_type = mime_types::extension_to_type(file_extension);
				if ((mime_type.find("text/") != std::string::npos) ||
					(mime_type.find("/xml") != std::string::npos) ||
					(mime_type.find("/javascript") != std::string::npos) ||
					(mime_type.find("/json") != std::string::npos)) {
					// Add charset on text content
					mime_type += ";charset=UTF-8";
				}
				reply::add_header_content_type(&rep, mime_type);
			}
			reply::add_header_attachment(&rep, attachment_name);
			reply::add_header(&rep, "Content-Length", std::to_string(total_size));

			//write headers
			std::string headers = rep.to_string("GET");
			write_buffer = headers;

			if (secure_) {
#ifdef WWW_ENABLE_SSL
				boost::asio::async_write(*sslsocket_, boost::asio::buffer(write_buffer), boost::bind(&connection::handle_write_file, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#endif
			}
			else {
				boost::asio::async_write(*socket_, boost::asio::buffer(write_buffer), boost::bind(&connection::handle_write_file, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
			return true;
		}

		void connection::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
		{
			status_ = READING;

			// data read, no need for timeouts (RK, note: race condition)
			cancel_read_timeout();

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
				const char* begin;
				// websocket variables
				size_t bytes_consumed;

				switch (connection_type)
				{
				case ConnectionType::connection_http:
					begin = boost::asio::buffer_cast<const char*>(_buf.data());
					try
					{
						request_parser_.reset();
						boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
							request_, begin, begin + _buf.size());
					}
					catch (...)
					{
						_log.Log(LOG_ERROR, "Exception parsing HTTP. Address: %s", host_endpoint_address_.c_str());
					}

					if (result) {
						size_t sizeread = begin - boost::asio::buffer_cast<const char*>(_buf.data());
						_buf.consume(sizeread);
						reply_.reset();
						const char* pConnection = request_.get_req_header(&request_, "Connection");
						keepalive_ = pConnection != nullptr && boost::iequals(pConnection, "Keep-Alive");
						request_.keep_alive = keepalive_;
						request_.host_address = host_endpoint_address_;
						request_.host_port = host_endpoint_port_;
						if (request_.host_address.substr(0, 7) == "::ffff:") {
							request_.host_address = request_.host_address.substr(7);
						}
						request_handler_.handle_request(request_, reply_);

						if (reply_.status == reply::switching_protocols) {
							// this was an upgrade request
							connection_type = ConnectionType::connection_websocket;
							// from now on we are a persistant connection
							keepalive_ = true;
							websocket_parser.Start();
							websocket_parser.GetHandler()->store_session_id(request_, reply_);
							// todo: check if multiple connection from the same client in CONNECTING state?
						}
						else if (reply_.status == reply::download_file) {
							std::string filename_attachment = reply_.content;
							size_t npos = filename_attachment.find("\r\n");
							if (npos == std::string::npos)
							{
								reply_ = reply::stock_reply(reply::internal_server_error);
							}
							else
							{
								std::string filename = filename_attachment.substr(0, npos);
								std::string attachment = filename_attachment.substr(npos + 2);
								if (send_file(filename, attachment, reply_))
									return;
							}
						}

						if (request_.keep_alive && ((reply_.status == reply::ok) || (reply_.status == reply::no_content) || (reply_.status == reply::not_modified))) {
							// Allows request handler to override the header (but it should not)
							reply::add_header_if_absent(&reply_, "Connection", "Keep-Alive");
							std::stringstream ss;
							ss << "max=" << default_max_requests_ << ", timeout=" << read_timeout_;
							reply::add_header_if_absent(&reply_, "Keep-Alive", ss.str());
						}

						MyWrite(reply_.to_string(request_.method));
						if (reply_.status == reply::switching_protocols) {
							// this was an upgrade request, set this value after MyWrite to allow the 101 response to go out
							connection_type = ConnectionType::connection_websocket;
						}

						if (keepalive_) {
							read_more();
						}
						status_ = WAITING_WRITE;
					}
					else if (!result)
					{
						_log.Log(LOG_ERROR, "Error parsing http request address: %s", host_endpoint_address_.c_str());
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
				case ConnectionType::connection_websocket:
				case ConnectionType::connection_websocket_closing:
					begin = boost::asio::buffer_cast<const char*>(_buf.data());
					result = websocket_parser.parse((const unsigned char*)begin, _buf.size(), bytes_consumed, keepalive_);
					_buf.consume(bytes_consumed);
					if (result) {
						// we received a complete packet (that was handled already)
						if (keepalive_) {
							read_more();
						}
						else {
							// a connection close control packet was received
							// todo: wait for writeQ to flush?
							connection_type = ConnectionType::connection_websocket_closing;
						}
					}
					else // if (!result)
					{
						read_more();
					}
					break;
				}
			}
			else if (error == boost::asio::error::eof)
			{
				connection_manager_.stop(shared_from_this());
			}
			else if (error != boost::asio::error::operation_aborted)
			{
				// _log.Log(LOG_ERROR, "connection::handle_read Error: %s", error.message().c_str());
				connection_manager_.stop(shared_from_this());
			}
		}

		void connection::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
		{
			std::unique_lock<std::mutex> lock(writeMutex);
			write_buffer.clear();
			write_in_progress = false;
			bool stopConnection = false;
			if (!error && !writeQ.empty())
			{
				std::string buf = writeQ.front();
				writeQ.pop_front();
				SocketWrite(buf);
				if (keepalive_)
				{
					reset_abandoned_timeout();
				}
				return;
			}

			//Stop needs to be outside the lock. 
			//There are flows it dead-locks in CWebSocketPush::Stop()
			lock.unlock();

			if (error == boost::asio::error::operation_aborted)
			{
				connection_manager_.stop(shared_from_this());
			}
			else if (error)
			{
				// _log.Log(LOG_ERROR, "connection::handle_write Error: %s", error.message().c_str());
				connection_manager_.stop(shared_from_this());
			}
			else if (keepalive_)
			{
				status_ = ENDING_WRITE;
				reset_abandoned_timeout();
			}
			else
			{
				//Everything has been send. Closing connection.
				connection_manager_.stop(shared_from_this());
			}
		}

		connection::~connection()
		{
			// free up resources, delete the socket pointers
			delete socket_;
#ifdef WWW_ENABLE_SSL
			delete sslsocket_;
#endif
			delete[] send_buffer_;
		}

		// schedule read timeout timer
		void connection::set_read_timeout() {
			read_timer_.expires_from_now(boost::posix_time::seconds(read_timeout_));
			read_timer_.async_wait(boost::bind(&connection::handle_read_timeout, shared_from_this(), boost::asio::placeholders::error));
		}

		/// simply cancel read timeout timer
		void connection::cancel_read_timeout() {
			try {
				boost::system::error_code ignored_ec;
				read_timer_.cancel(ignored_ec);
				if (ignored_ec) {
					_log.Log(LOG_ERROR, "%s -> exception thrown while canceling read timeout : %s", host_endpoint_address_.c_str(), ignored_ec.message().c_str());
				}
			}
			catch (...) {
				_log.Log(LOG_ERROR, "%s -> exception thrown while canceling read timeout", host_endpoint_address_.c_str());
			}
		}

		/// reschedule read timeout timer
		void connection::reset_read_timeout() {
			cancel_read_timeout();
			set_read_timeout();
		}

		/// stop connection on read timeout
		void connection::handle_read_timeout(const boost::system::error_code& error) {
			if (!error && keepalive_ && (connection_type == ConnectionType::connection_websocket)) {
				// For WebSockets that requested keep-alive, use a Server side Ping
				websocket_parser.SendPing();
			}
			else if (!error)
			{
				connection_manager_.stop(shared_from_this());
			}
			else if (error != boost::asio::error::operation_aborted)
			{
				_log.Log(LOG_ERROR, "connection::handle_read_timeout Error: %s", error.message().c_str());
				connection_manager_.stop(shared_from_this());
			}
		}

		/// schedule abandoned timeout timer
		void connection::set_abandoned_timeout() {
			abandoned_timer_.expires_from_now(boost::posix_time::seconds(default_abandoned_timeout_));
			abandoned_timer_.async_wait(boost::bind(&connection::handle_abandoned_timeout, shared_from_this(), boost::asio::placeholders::error));
		}

		/// simply cancel abandoned timeout timer
		void connection::cancel_abandoned_timeout() {
			try {
				boost::system::error_code ignored_ec;
				abandoned_timer_.cancel(ignored_ec);
				if (ignored_ec) {
					_log.Log(LOG_ERROR, "%s -> exception thrown while canceling abandoned timeout : %s", host_endpoint_address_.c_str(), ignored_ec.message().c_str());
				}
			}
			catch (...) {
				_log.Log(LOG_ERROR, "%s -> exception thrown while canceling abandoned timeout", host_endpoint_address_.c_str());
			}
		}

		/// reschedule abandoned timeout timer
		void connection::reset_abandoned_timeout() {
			cancel_abandoned_timeout();
			set_abandoned_timeout();
		}

		/// stop connection on abandoned timeout
		void connection::handle_abandoned_timeout(const boost::system::error_code& error) {
			if (error != boost::asio::error::operation_aborted) {
				_log.Log(LOG_STATUS, "%s -> handle abandoned timeout (status=%d)", host_endpoint_address_.c_str(), status_);
				connection_manager_.stop(shared_from_this());
			}
		}

	} // namespace server
} // namespace http
