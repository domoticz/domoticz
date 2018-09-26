#pragma once

#include <stddef.h>                        // for size_t
#include <boost/asio/deadline_timer.hpp>   // for deadline_timer
#include <boost/asio/io_service.hpp>       // for io_service
#include <boost/asio/ip/tcp.hpp>           // for tcp, tcp::endpoint, tcp::s...
#include <boost/function.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr
#include <exception>                       // for exception

#define ASYNCTCP_THREAD_NAME "ASyncTCP"

namespace boost { namespace system { class error_code; } }

class ASyncTCP
{
protected:
	ASyncTCP();
	virtual ~ASyncTCP(void);
	// Async connect and start async read from the socket, data is delivered in OnData()
	// No action if mIsConnected = true. If the connection is lost, it will automatically be setup again
	void connect(const std::string &hostname, unsigned short port);

	// Schedule reconnect (TODO: Implement)
	void schedule_reconnect(const int Delay = 30);

	bool isConnected() { return mIsConnected; };

	// Async disconnect from the socket, no action if mIsConnected = false
	void disconnect(const bool silent = true);

	// Disable callback, async disconnect from the socket
	void terminate(const bool silent = true);

	// Async write to the socket, no action if mIsConnected = false
	void write(const std::string &msg);

	// Async write to the socket, no action if mIsConnected = false
	void write(const uint8_t *pData, size_t length);

	// Set reconnect delay in seconds, set to 0 to disable reconnect
	void SetReconnectDelay(const int Delay = 30);

	// Callback interface to implement in derived classes
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnData(const unsigned char *pData, size_t length) = 0;
	virtual void OnError(const std::exception e) = 0;
	virtual void OnError(const boost::system::error_code& error) = 0;

protected:
	boost::asio::io_service			mIos; // protected to allow derived classes to attach timers etc.

private:
	// Internal helper functions
	void connect(boost::asio::ip::tcp::endpoint& endpoint);
	void close();
	void read();
	void OnErrorInt(const boost::system::error_code& error);
	void StartReconnect();

	// Callbacks for the io_service, executed from the io_service worker thread context
	void handle_connect(const boost::system::error_code& error);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void write_end(const boost::system::error_code& error);
	void do_close();
	void do_reconnect(const boost::system::error_code& error);
	void do_write(const std::string &msg);

	// State variables
	bool							mIsConnected;
	bool							mIsClosing;
	bool							mDoReconnect;
	bool							mIsReconnecting;
	bool							mAllowCallbacks;

	// Internal
	unsigned char 					m_rx_buffer[1024];

	int								m_reconnect_delay;
	boost::asio::deadline_timer		mReconnectTimer;

	std::shared_ptr<std::thread> 	m_tcpthread;
	boost::asio::io_service::work 	m_tcpwork; // Create some work to keep IO Service alive

	boost::asio::ip::tcp::socket	mSocket;
	boost::asio::ip::tcp::endpoint	mEndPoint;
};
