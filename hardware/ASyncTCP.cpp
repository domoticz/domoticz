#include "stdafx.h"
#include "ASyncTCP.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>     // for error_code
#include "../main/Helper.h"                // for SetThreadName
#include "../main/Logger.h"                // for CLogger, _log, _eLogLevel:...
struct hostent;

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

#define RECONNECT_TIME 30

// TODO: Delete default constructor and require thread name to be set
ASyncTCP::ASyncTCP(const std::string thread_name)
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mReconnectTimer(mIos),
	mDoReconnect(true), mIsReconnecting(false),
	m_tcpwork(mIos)
{
	// Reset IO Service
	mIos.reset();

	//Start IO Service worker thread
	m_tcpthread = std::make_shared<std::thread>(boost::bind(&boost::asio::io_service::run, &mIos));
	SetThreadName(m_tcpthread->native_handle(), thread_name.c_str());
}

ASyncTCP::~ASyncTCP(void)
{
	disconnect();

	// tell the IO service to stop
	mIos.stop();
	try {
		if (m_tcpthread)
		{
			m_tcpthread->join();
			m_tcpthread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
}

void ASyncTCP::connect(const std::string &ip, unsigned short port)
{
	// connect socket
	try
	{
		std::string fip = ip;

		unsigned long ipn = inet_addr(fip.c_str());
		// if we have a error in the ip, it means we have entered a string
		if (ipn == INADDR_NONE)
		{
			// change Hostname in Server Address
			hostent *he = gethostbyname(fip.c_str());
			if (he != NULL)
			{
				char szIP[20];
				sprintf(szIP, "%d.%d.%d.%d", (uint8_t)he->h_addr_list[0][0], (uint8_t)he->h_addr_list[0][1], (uint8_t)he->h_addr_list[0][2], (uint8_t)he->h_addr_list[0][3]);
				fip = szIP;
			}
			else
			{
				//we will fail
				_log.Log(LOG_ERROR, "TCP: Unable to resolve '%s'", fip.c_str());
			}
		}

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(fip), port);

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
	if (!mIsConnected) return;
	if (mIsClosing) return;

	mSocket.async_read_some(boost::asio::buffer(m_readbuffer, sizeof(m_readbuffer)),
		boost::bind(&ASyncTCP::handle_read,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void ASyncTCP::write(const uint8_t *pData, size_t length)
{
	write(std::string((const char*)pData, length));
}

void ASyncTCP::write(const std::string &msg)
{
	mIos.post(boost::bind(&ASyncTCP::do_write, this, msg));
}

// callbacks

void ASyncTCP::handle_connect(const boost::system::error_code& error)
{
	if(mIsClosing) return;

	if (!error) {
		// we are connected!
		mIsConnected = true;

		//Enable keep alive
		boost::asio::socket_base::keep_alive option(true);
		mSocket.set_option(option);

		//set_tcp_keepalive();

		OnConnect();

		// Start Reading
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncTCP::read, this));
	}
	else {
		// there was an error :(
		mIsConnected = false;

		OnError(error);
		OnErrorInt(error);

		if (!mDoReconnect)
		{
			OnDisconnect();
			return;
		}
		if (!mIsReconnecting)
		{
			mIsReconnecting = true;
			_log.Log(LOG_STATUS, "TCP: Reconnecting in %d seconds...", RECONNECT_TIME);
			// schedule a timer to reconnect after 30 seconds
			mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
			mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
		}
	}
}

void ASyncTCP::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		OnData(m_readbuffer,bytes_transferred);
		//Read next
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncTCP::read, this));
	}
	else
	{
		// try to reconnect if external host disconnects
		if (!mIsClosing)
		{
			mIsConnected = false;

			// let listeners know
			OnError(error);
			if (!mDoReconnect)
			{
				OnDisconnect();
				return;
			}
			if (!mIsReconnecting)
			{
				mIsReconnecting = true;
				_log.Log(LOG_STATUS, "TCP: Reconnecting in %d seconds...", RECONNECT_TIME);
				// schedule a timer to reconnect after 30 seconds
				mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
				mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
			}
		}
		else
			do_close();
	}
}

void ASyncTCP::write_end(const boost::system::error_code& error)
{
	if (!mIsClosing)
	{
		if (error)
		{
			// let listeners know
			OnError(error);

			mIsConnected = false;

			if (!mDoReconnect)
			{
				OnDisconnect();
				return;
			}
			if (!mIsReconnecting)
			{
				mIsReconnecting = true;
				_log.Log(LOG_STATUS, "TCP: Reconnecting in %d seconds...", RECONNECT_TIME);
				// schedule a timer to reconnect after 30 seconds
				mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
				mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
			}
		}
	}
}

void ASyncTCP::do_close()
{
	if(mIsClosing) return;

	mIsClosing = true;

	mSocket.close();
}

void ASyncTCP::do_reconnect(const boost::system::error_code& /*error*/)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	// close current socket if necessary
	mSocket.close();

	if (!mDoReconnect)
	{
		return;
	}
	mReconnectTimer.cancel();
	// try to reconnect, then call handle_connect
	_log.Log(LOG_STATUS, "TCP: Reconnecting...");
	mSocket.async_connect(mEndPoint,
        boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
	mIsReconnecting = false;
}

void ASyncTCP::do_write(const std::string &msg)
{
	if(!mIsConnected) return;

	if (!mIsClosing)
	{
		boost::asio::async_write(mSocket,
			boost::asio::buffer(msg.c_str(), msg.size()),
			boost::bind(&ASyncTCP::write_end, this, boost::asio::placeholders::error));
	}
}

void ASyncTCP::OnErrorInt(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_STATUS, "TCP: Connection problem (Unable to connect to specified IP/Port)");
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "TCP: Connection reset! (Disconnected)");
	}
	else
		_log.Log(LOG_ERROR, "TCP: Error: %s", error.message().c_str());
}
