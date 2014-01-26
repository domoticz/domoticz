#pragma once

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

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

protected:
	bool							mIsConnected;
	bool							mIsClosing;

	boost::asio::ip::tcp::endpoint	mEndPoint;

	boost::asio::io_service			mIos;
	boost::asio::ip::tcp::socket	mSocket;

	static const int readBufferSize=1024;
	unsigned char m_buffer[readBufferSize];

	boost::asio::deadline_timer		mReconnectTimer;

};
