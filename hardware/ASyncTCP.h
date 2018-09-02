#pragma once

#include <stddef.h>                        // for size_t
#include <boost/asio/deadline_timer.hpp>   // for deadline_timer
#include <boost/asio/io_service.hpp>       // for io_service
#include <boost/asio/ip/tcp.hpp>           // for tcp, tcp::endpoint, tcp::s...
#include <boost/function.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr
#include <exception>                       // for exception

namespace boost { namespace system { class error_code; } }

typedef std::shared_ptr<class ASyncTCP> ASyncTCPRef;

class ASyncTCP
{
public:
	ASyncTCP();
	virtual ~ASyncTCP(void);

	void connect(const std::string &ip, unsigned short port);
	void connect(boost::asio::ip::tcp::endpoint& endpoint);
	bool isConnected() { return mIsConnected; };
	void disconnect(const bool silent = true);
	void terminate(const bool silent = true);

	void write(const std::string &msg);
	void write(const uint8_t *pData, size_t length);

	void update();

	void SetReconnectDelay(int Delay = 30); //0 disabled retry
protected:
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnData(const unsigned char *pData, size_t length) = 0;
	virtual void OnError(const std::exception e) = 0;
	virtual void OnError(const boost::system::error_code& error) = 0;
private:
	void read();
	void close();

	//bool set_tcp_keepalive();

	// callbacks
	void handle_connect(const boost::system::error_code& error);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void write_end(const boost::system::error_code& error);
	void do_close();

	void do_reconnect(const boost::system::error_code& error);
	void OnErrorInt(const boost::system::error_code& error);
	void StartReconnect();
	int m_reconnect_delay;
	bool							mIsConnected;
	bool							mIsClosing;
	bool							mDoReconnect;
	bool							mIsReconnecting;
	bool							mAllowCallbacks;

	boost::asio::ip::tcp::endpoint	mEndPoint;

	boost::asio::io_service			mIos;
	boost::asio::ip::tcp::socket	mSocket;

	unsigned char m_rx_buffer[1024];

	boost::asio::deadline_timer		mReconnectTimer;

};
