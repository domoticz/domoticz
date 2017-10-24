#pragma once

#include <stddef.h>                        // for size_t
#include <boost/asio/deadline_timer.hpp>   // for deadline_timer
#include <boost/asio/io_service.hpp>       // for io_service
#include <boost/asio/ip/tcp.hpp>           // for tcp, tcp::endpoint, tcp::s...
#include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr
#include <exception>                       // for exception
#include <iosfwd>                        // for string
namespace boost { namespace system { class error_code; } }

typedef boost::shared_ptr<class ASyncTCP> ASyncTCPRef;

class ASyncTCP
{
public:
	ASyncTCP();
	virtual ~ASyncTCP(void);

	void write(const std::string &msg);

	void connect(const std::string &ip, unsigned short port);
	void connect(boost::asio::ip::tcp::endpoint& endpoint);

	void disconnect();

	bool isConnected(){ return mIsConnected; };

	void update();
protected:
	void read();
	void close();
	//bool set_tcp_keepalive();

	// callbacks
	void handle_connect(const boost::system::error_code& error);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void write_end(const boost::system::error_code& error);

	void write(const unsigned char *pData, size_t length);
	void do_close();

	void do_reconnect(const boost::system::error_code& error);

	virtual void OnConnect()=0;
	virtual void OnDisconnect()=0;
	virtual void OnData(const unsigned char *pData, size_t length)=0;
	virtual void OnError(const std::exception e)=0;
	virtual void OnError(const boost::system::error_code& error)=0;

	void OnErrorInt(const boost::system::error_code& error);

protected:
	bool							mIsConnected;
	bool							mIsClosing;
	bool							mDoReconnect;
	bool							mIsReconnecting;

	boost::asio::ip::tcp::endpoint	mEndPoint;

	boost::asio::io_service			mIos;
	boost::asio::ip::tcp::socket	mSocket;

	unsigned char m_buffer[1024];

	boost::asio::deadline_timer		mReconnectTimer;

};
