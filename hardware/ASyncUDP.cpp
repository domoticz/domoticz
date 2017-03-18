#include "stdafx.h"
#include "ASyncUDP.h"
#include "../main/Logger.h"

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

ASyncUDP::ASyncUDP()
	: mIsConnected(false), mIsClosing(false),
	mSocket(mIos), mReconnectTimer(mIos),
	mDoReconnect(true), mIsReconnecting(false)
{
}

ASyncUDP::~ASyncUDP(void)
{
	disconnect();
}

void ASyncUDP::update()
{
	if (mIsClosing)
		return;
	// calls the poll() function to process network messages
	mIos.poll();
}

void ASyncUDP::connect(const std::string &ip, unsigned short port)
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
				_log.Log(LOG_ERROR, "UDP: Unable to resolve '%s'", fip.c_str());
			}
		}

		boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(fip), port);

		connect(endpoint);
	}
	catch(const std::exception &e)
	{
		OnError(e);
		_log.Log(LOG_ERROR,"UDP: Exception: %s", e.what());
	}
}

void ASyncUDP::connect(boost::asio::ip::udp::endpoint& endpoint)
{
	//if(mIsConnected) return;
	//if(mIsClosing) return;

	mEndPoint = endpoint;

	// try to connect, then call handle_connect
	mSocket.async_connect(endpoint,
        boost::bind(&ASyncUDP::handle_connect, this, boost::asio::placeholders::error));
}

void ASyncUDP::disconnect()
{
	// tell socket to close the connection
	close();

	// tell the IO service to stop
	mIos.stop();

	//mIsConnected = false;
	mIsClosing = false;
}

void ASyncUDP::close()
{
	//if(!mIsConnected) return;

	// safe way to request the client to close the connection
	mIos.post(boost::bind(&ASyncUDP::do_close, this));
}

// callbacks

void ASyncUDP::handle_connect(const boost::system::error_code& error)
{

	if(mIsClosing) return;

	if (!error) {

		// we are connected!
		//mIsConnected = true;

		//Enable keep alive
		//boost::asio::socket_base::keep_alive option(true);
		//mSocket.set_option(option);

		//set_tcp_keepalive();

		OnConnect();

		// Start Reading
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncUDP::read, this));
	}
	else {
		// there was an error :(
		// mIsConnected = false;

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
			_log.Log(LOG_STATUS, "UDP: Reconnecting in %d seconds...", RECONNECT_TIME);
			// schedule a timer to reconnect after 30 seconds
			mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
			mReconnectTimer.async_wait(boost::bind(&ASyncUDP::do_reconnect, this, boost::asio::placeholders::error));
		}
	}
}

void ASyncUDP::read()
{
//	if (!mIsConnected) return;
//	if (mIsClosing) return;

	mSocket.async_receive_from(boost::asio::buffer(m_buffer, sizeof(m_buffer)),
          mEndPoint,
		boost::bind(&ASyncUDP::handle_read,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void ASyncUDP::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		OnData(m_buffer,bytes_transferred);
		//Read next
		//This gives some work to the io_service before it is started
		mIos.post(boost::bind(&ASyncUDP::read, this));
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
				_log.Log(LOG_STATUS, "UDP: Reconnecting in %d seconds...", RECONNECT_TIME);
				// schedule a timer to reconnect after 30 seconds
				mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
				mReconnectTimer.async_wait(boost::bind(&ASyncUDP::do_reconnect, this, boost::asio::placeholders::error));
			}
		}
		else
			do_close();
	}
}

void ASyncUDP::write_end(const boost::system::error_code& error)
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
				_log.Log(LOG_STATUS, "UDP: Reconnecting in %d seconds...", RECONNECT_TIME);
				// schedule a timer to reconnect after 30 seconds
				mReconnectTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_TIME));
				mReconnectTimer.async_wait(boost::bind(&ASyncUDP::do_reconnect, this, boost::asio::placeholders::error));
			}
		}
	}
}



void ASyncUDP::write(const unsigned char *pData, size_t length)
{

	if (!mIsClosing)
	{
		  mSocket.async_send_to(boost::asio::buffer(pData,length),
                            mEndPoint,
		                        boost::bind(&ASyncUDP::write_end,
			this,
			boost::asio::placeholders::error));
	}
}

void ASyncUDP::write(const std::string &msg)
{
	write((const unsigned char*)msg.c_str(), msg.size());
}

void ASyncUDP::do_close()
{
	if(mIsClosing) return;

	mIsClosing = true;

	mSocket.close();
}

void ASyncUDP::do_reconnect(const boost::system::error_code& error)
{
	//if(mIsConnected) return;
	if(mIsClosing) return;

	// close current socket if necessary
	mSocket.close();

	mReconnectTimer.cancel();
	// try to reconnect, then call handle_connect
	_log.Log(LOG_STATUS, "UDP: Reconnecting...");
	mSocket.async_connect(mEndPoint,
        boost::bind(&ASyncUDP::handle_connect, this, boost::asio::placeholders::error));
	mIsReconnecting = false;
}

void ASyncUDP::OnErrorInt(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_STATUS, "UDP: Connection problem (Unable to connect to specified IP/Port)");
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "UDP: Connection reset! (Disconnected)");
	}
	else
		_log.Log(LOG_ERROR, "UDP: Error: %s", error.message().c_str());
}
