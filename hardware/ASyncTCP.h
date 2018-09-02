#pragma once

#include <stddef.h>                        // for size_t
#include <boost/asio/deadline_timer.hpp>   // for deadline_timer
#include <boost/asio/io_service.hpp>       // for io_service
#include <boost/asio/ip/tcp.hpp>           // for tcp, tcp::endpoint, tcp::s...
#include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr
#include <exception>                       // for exception
namespace boost { namespace system { class error_code; } }

typedef std::shared_ptr<class ASyncTCP> ASyncTCPRef;

class ASyncTCP
{
protected:
	ASyncTCP() = delete;
	ASyncTCP(const std::string thread_name);
	virtual ~ASyncTCP(void);

	void connect(const std::string &hostname, unsigned short port); // Async connect to the socket, no action if mIsConnected = true. If the connection is lost, it will automatically be setup again
	void scedule_reconnect(const int millis);         // Schedule reconnect (TODO: Implement)
	void disconnect();                                // Async disconnect from the socket, no action if mIsConnected = false
	bool isConnected() { return mIsConnected; };

	void write(const std::string &msg);               // Async write to the socket, no action if mIsConnected = false
	void write(const uint8_t *pData, size_t length);  // Async write to the socket, no action if mIsConnected = false

	// Callback interface to implement in derived classes
	virtual void OnConnect()=0;
	virtual void OnDisconnect()=0;
	virtual void OnData(const unsigned char *pData, size_t length)=0;
	virtual void OnError(const std::exception e)=0;
	virtual void OnError(const boost::system::error_code& error)=0;

protected:
	boost::asio::io_service			mIos; // protected to allow derived classes to attach timers etc.

private:
	// Internal helper function
	void connect(boost::asio::ip::tcp::endpoint& endpoint);
	void OnErrorInt(const boost::system::error_code& error);
	void close();
	void read(); // Start async read from the socket, data is delivered in OnData(), no action if mIsConnected = false

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

	// Internal
	unsigned char 					m_readbuffer[1024];

	boost::asio::deadline_timer		mReconnectTimer;

	std::shared_ptr<std::thread> 	m_tcpthread;
	boost::asio::io_service::work 	m_tcpwork; // Create some work to keep IO Service alive

	boost::asio::ip::tcp::socket	mSocket;
	boost::asio::ip::tcp::endpoint	mEndPoint;
};
