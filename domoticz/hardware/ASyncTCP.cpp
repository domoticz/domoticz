#include "stdafx.h"
#include "ASyncTCP.h"
#include "../main/Logger.h"

#define RECONNECT_TIME 30

ASyncTCP::ASyncTCP()
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mReconnectTimer(mIos)
{	
}

ASyncTCP::~ASyncTCP(void)
{
	disconnect();
}

void ASyncTCP::update()
{
	// calls the poll() function to process network messages
	mIos.poll();
}

void ASyncTCP::connect(const std::string &ip, unsigned short port)
{
	// connect socket
	try 
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

		connect(endpoint);
	}
	catch(const std::exception &e) 
	{
		OnError(e);
		_log.Log(LOG_ERROR,"TCP: Exception: %s", e.what());
	}
}

void ASyncTCP::connect(boost::asio::ip::tcp::endpoint& endpoint)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	mEndPoint = endpoint;

	// try to connect, then call handle_connect
	mSocket.async_connect(endpoint,
        boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
}

void ASyncTCP::disconnect()
{		
	// tell socket to close the connection
	close();
	
	// tell the IO service to stop
	mIos.stop();

	mIsConnected = false;
	mIsClosing = false;
}

void ASyncTCP::close()
{
	if(!mIsConnected) return;

	// safe way to request the client to close the connection
	mIos.post(boost::bind(&ASyncTCP::do_close, this));
}

void ASyncTCP::read()
{
	if(!mIsConnected) return;
	if(mIsClosing) return;

	mSocket.async_read_some(boost::asio::buffer(m_buffer,readBufferSize),
		boost::bind(&ASyncTCP::handle_read,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

// callbacks

void ASyncTCP::handle_connect(const boost::system::error_code& error) 
{
	if(mIsClosing) return;
	
	if (!error) {
		// we are connected!
		mIsConnected = true;

		OnConnect();

		// Start Reading
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncTCP::read, this));
	}
	else {
		// there was an error :(
		mIsConnected = false;

		OnError(error);
		_log.Log(LOG_ERROR,"TCP: Error: %s", error.message().c_str());

		// schedule a timer to reconnect after 30 seconds		
		mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
		mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
	}
}

void ASyncTCP::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		//Read next
		OnData(m_buffer,bytes_transferred);
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncTCP::read, this));
	}
	else
	{
		// try to reconnect if external host disconnects
		if(error.value() != 0) {
			mIsConnected = false;

			// let listeners know
			OnError(error);
			
			// schedule a timer to reconnect after 30 seconds
			mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
			mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
		}
		else
			do_close();
	}
}

void ASyncTCP::write_end(const boost::system::error_code& error)
{
}

void ASyncTCP::write(const unsigned char *pData, size_t length)
{
	if(!mIsConnected) return;

	if (!mIsClosing)
	{
		boost::asio::async_write(mSocket,
			boost::asio::buffer(pData,length),
			boost::bind(&ASyncTCP::write_end, this, boost::asio::placeholders::error));
	}
}

void ASyncTCP::do_close()
{
	if(mIsClosing) return;
	
	mIsClosing = true;

	mSocket.close();
}

void ASyncTCP::do_reconnect(const boost::system::error_code& error)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	// close current socket if necessary
	mSocket.close();

	// try to reconnect, then call handle_connect
	mSocket.async_connect(mEndPoint,
        boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
}
