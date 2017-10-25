#include "stdafx.h"
#include "ASyncTCP.h"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>     // for error_code
#include "../main/Logger.h"                // for CLogger, _log, _eLogLevel:...
struct hostent;

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

/*
#ifdef WIN32
	#include <Mstcpip.h>
#elif defined(__FreeBSD__)
	#include <netinet/tcp.h>
#endif
*/

#define RECONNECT_TIME 30

ASyncTCP::ASyncTCP()
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mReconnectTimer(mIos),
	mDoReconnect(true), mIsReconnecting(false)
{	
}

ASyncTCP::~ASyncTCP(void)
{
	disconnect();
}

void ASyncTCP::update()
{
	if (mIsClosing)
		return;
	// calls the poll() function to process network messages
	mIos.poll();
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

/*
bool ASyncTCP::set_tcp_keepalive()
{
	int keep_alive_timeout = 10;

#ifdef __OSX__
	int native_fd = socket->native();
	int timeout = *keep_alive_timeout;
	int intvl = 1;
	int on = 1;

	// Set the timeout before the first keep alive message
	int ret_sokeepalive = setsockopt(native_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(int));
	int ret_tcpkeepalive = setsockopt(native_fd, IPPROTO_TCP, TCP_KEEPALIVE, (void*)&timeout, sizeof(int));
	int ret_tcpkeepintvl = setsockopt(native_fd, IPPROTO_TCP, TCP_CONNECTIONTIMEOUT, (void*)&intvl, sizeof(int));

	if (ret_sokeepalive || ret_tcpkeepalive || ret_tcpkeepintvl)
	{
		string message("Failed to enable keep alive on TCP client socket!");
		Logger::error(message, port, host);
		return false;
	}
#elif defined(WIN32)
	// Partially supported on windows
	struct tcp_keepalive keepalive_options;
	keepalive_options.onoff = 1;
	keepalive_options.keepalivetime = keep_alive_timeout * 1000;
	keepalive_options.keepaliveinterval = 2000;

	BOOL keepalive_val = true;
	SOCKET native = mSocket.native();
	DWORD bytes_returned;

	int ret_keepalive = setsockopt(native, SOL_SOCKET, SO_KEEPALIVE, (const char *)&keepalive_val, sizeof(keepalive_val));
	int ret_iotcl = WSAIoctl(native, SIO_KEEPALIVE_VALS, (LPVOID)& keepalive_options, (DWORD) sizeof(keepalive_options), NULL, 0,
		(LPDWORD)& bytes_returned, NULL, NULL);

	if (ret_keepalive || ret_iotcl)
	{
		_log.Log(LOG_ERROR, "Failed to set keep alive timeout on TCP client socket!");
		return false;
	}
#else
	// For *n*x systems
	int native_fd = mSocket.native();
	int timeout = keep_alive_timeout;
	int intvl = 1;
	int probes = 10;
	int on = 1;

	int ret_keepalive = setsockopt(native_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&on, sizeof(int));
	int ret_keepidle = setsockopt(native_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&timeout, sizeof(int));
	int ret_keepintvl = setsockopt(native_fd, SOL_TCP, TCP_KEEPINTVL, (void*)&intvl, sizeof(int));
	int ret_keepinit = setsockopt(native_fd, SOL_TCP, TCP_KEEPCNT, (void*)&probes, sizeof(int));

	if (ret_keepalive || ret_keepidle || ret_keepintvl || ret_keepinit)
	{
		_log.Log(LOG_ERROR, "Failed to set keep alive timeout on TCP client socket!");
		return false;
	}
#endif
	return true;
}
*/

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

void ASyncTCP::read()
{
	if (!mIsConnected) return;
	if (mIsClosing) return;

	mSocket.async_read_some(boost::asio::buffer(m_buffer, sizeof(m_buffer)),
		boost::bind(&ASyncTCP::handle_read,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void ASyncTCP::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		OnData(m_buffer,bytes_transferred);
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

void ASyncTCP::write(const std::string &msg)
{
	write((const unsigned char*)msg.c_str(), msg.size());
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
