#include "stdafx.h"
#include "ASyncTCP.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>     // for error_code
struct hostent;

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

#define Q_UNUSED(x) (void)x;

#define RECONNECT_TIME 30

ASyncTCP::ASyncTCP(const bool secure)
	: mIsConnected(false), mIsClosing(false), mWriteInProgress(false),
#ifdef WWW_ENABLE_SSL
	mSecure(secure), m_Context(boost::asio::ssl::context::sslv23),
#endif
	m_Socket(mIos), mReconnectTimer(mIos),
#ifdef WWW_ENABLE_SSL
	mSslSocket(mIos, m_Context),
#endif
	mDoReconnect(true), mIsReconnecting(false),
	m_tcpwork(mIos),
	mAllowCallbacks(true),
	m_reconnect_delay(RECONNECT_TIME)
{
	// Reset IO Service
	mIos.reset();

	//Start IO Service worker thread
	m_tcpthread = std::make_shared<std::thread>(boost::bind(&boost::asio::io_service::run, &mIos));

#ifdef WWW_ENABLE_SSL
	// we do not authenticate the server
	m_Context.set_verify_mode(boost::asio::ssl::verify_none);
#endif
}

ASyncTCP::~ASyncTCP(void)
{
	disconnect();

	// tell the IO service to stop
	m_tcpwork = boost::none;
	if (m_tcpthread)
	{
		m_tcpthread->join();
		m_tcpthread.reset();
	}
}

void ASyncTCP::SetReconnectDelay(int Delay)
{
	m_reconnect_delay = Delay;
}

void ASyncTCP::connect(const std::string &ip, unsigned short port)
{
	std::stringstream fip;
	// resolve hostname
	try
	{
		boost::asio::ip::tcp::resolver resolver(mIos);
		boost::asio::ip::tcp::resolver::query query(ip, "");
		for(auto i = resolver.resolve(query); i != boost::asio::ip::tcp::resolver::iterator(); ++i)
		{
			boost::asio::ip::tcp::endpoint end = *i;
			fip << end.address();
			break; // only retrieve the first address
		}
	}
	catch (const std::exception &e)
	{
		Q_UNUSED(e);
		if (!mAllowCallbacks)
			return;
		OnError(boost::system::error_code(boost::asio::error::host_not_found));
	}

	// connect socket
	try
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(fip.str()), port);
		connect(endpoint);
	}
	catch(const std::exception &e)
	{
		if (mAllowCallbacks)
			OnError(e);
	}
}

void ASyncTCP::connect(boost::asio::ip::tcp::endpoint& endpoint)
{
	if(mIsConnected) return;
	if(mIsClosing) return;

	mAllowCallbacks = true;

	m_EndPoint = endpoint;

#ifdef WWW_ENABLE_SSL
	// try to connect, then call handle_connect
	if (mSecure) {
		mSslSocket.lowest_layer().async_connect(endpoint,
			boost::bind(&ASyncTCP::handle_connect, this,
				boost::asio::placeholders::error));
	}
	else
#endif
	{
		m_Socket.async_connect(endpoint,
			boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
	}
}

void ASyncTCP::disconnect(const bool silent)
{
	mReconnectTimer.cancel();
	try
	{
		// tell socket to close the connection
		close();

		mIsConnected = false;
		mIsClosing = false;
	}
	catch (...)
	{
		if (silent == false) {
			throw;
		}
	}
}

void ASyncTCP::terminate(const bool silent)
{
	mAllowCallbacks = false;
	disconnect(silent);
}

void ASyncTCP::StartReconnect()
{
	if (m_reconnect_delay != 0)
	{
		mIsReconnecting = true;
		// schedule a timer to reconnect after xx seconds
		mReconnectTimer.expires_from_now(boost::posix_time::seconds(m_reconnect_delay));
		mReconnectTimer.async_wait(boost::bind(&ASyncTCP::do_reconnect, this, boost::asio::placeholders::error));
	}
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

#ifdef WWW_ENABLE_SSL
	if (mSecure) {
		mSslSocket.async_read_some(boost::asio::buffer(m_rx_buffer, sizeof(m_rx_buffer)),
			boost::bind(&ASyncTCP::handle_read,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else
#endif
	{
		m_Socket.async_read_some(boost::asio::buffer(m_rx_buffer, sizeof(m_rx_buffer)),
			boost::bind(&ASyncTCP::handle_read,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void ASyncTCP::write(const uint8_t *pData, size_t length)
{
	write(std::string((const char*)pData, length));
}

void ASyncTCP::write(const std::string &msg)
{
	boost::unique_lock<boost::mutex> lock(m_writeMutex);
	if (mWriteInProgress) {
		m_writeQ.push_back(msg);
	}
	else {
		mWriteInProgress = true;
		//do_write(msg);
		mIos.post(boost::bind(&ASyncTCP::do_write, this, msg));
	}
}

// callbacks

void ASyncTCP::handle_connect(const boost::system::error_code& error)
{
	if(mIsClosing) return;

	if (!error) {
#ifdef WWW_ENABLE_SSL
		if (mSecure) {
			// start ssl handshake to server
			mSslSocket.async_handshake(boost::asio::ssl::stream_base::client,
				boost::bind(&ASyncTCP::handle_handshake, this,
					boost::asio::placeholders::error));
		}
		else
#endif
		{
			// we are connected!
			mIsConnected = true;

			//Enable keep alive
			boost::asio::socket_base::keep_alive option(true);
			m_Socket.set_option(option);

			//set_tcp_keepalive();

			if (mAllowCallbacks)
				OnConnect();

			// Start Reading
			//This gives some work to the io_service before it is started
			mIos.post(boost::bind(&ASyncTCP::read, this));
		}

	}
	else {
		// there was an error :(
		mIsConnected = false;

		if (mAllowCallbacks)
			OnError(error);

		if (!mDoReconnect)
		{
			if (mAllowCallbacks)
				OnDisconnect();
			return;
		}
		if (!mIsReconnecting)
		{
			StartReconnect();
		}
	}
}

#ifdef WWW_ENABLE_SSL
void ASyncTCP::handle_handshake(const boost::system::error_code& error)
{
	// we are connected!
	mIsConnected = true;

	if (mAllowCallbacks)
		OnConnect();

	// Start Reading
	//This gives some work to the io_service before it is started
	mIos.post(boost::bind(&ASyncTCP::read, this));
}
#endif

void ASyncTCP::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error)
	{
		if (mAllowCallbacks)
			OnData(m_rx_buffer,bytes_transferred);
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
			if (mAllowCallbacks)
				OnError(error);
			if (!mDoReconnect)
			{
				if (mAllowCallbacks)
					OnDisconnect();
				return;
			}
			if (!mIsReconnecting)
			{
				StartReconnect();
			}
		}
	}
}

void ASyncTCP::write_end(const boost::system::error_code& error)
{
	if (!mIsClosing)
	{
		if (error)
		{
			// let listeners know
			if (mAllowCallbacks)
				OnError(error);

			mIsConnected = false;

			if (!mDoReconnect)
			{
				if (mAllowCallbacks)
					OnDisconnect();
				return;
			}
			if (!mIsReconnecting)
			{
				StartReconnect();
			}
		}
		else {
			boost::unique_lock<boost::mutex> lock(m_writeMutex);
			if (!m_writeQ.empty()) {
				std::string msg = m_writeQ.front();
				m_writeQ.pop_front();
				mIos.post(boost::bind(&ASyncTCP::do_write, this, msg));
				//do_write(msg);
			}
			else {
				mWriteInProgress = false;
			}
		}
	}
}

void ASyncTCP::do_close()
{
	if(mIsClosing) return;

	mIsClosing = true;

#ifdef WWW_ENABLE_SSL
	if (mSecure) {
		mSslSocket.lowest_layer().close();
	}
	else
#endif
	{
		m_Socket.close();
	}
}

void ASyncTCP::do_reconnect(const boost::system::error_code& err)
{
	if(mIsConnected) return;
	if(mIsClosing) return;
	if (err) return;

#ifdef WWW_ENABLE_SSL
	// close current socket if necessary
	mSslSocket.lowest_layer().close();
#endif
	m_Socket.close();

	if (!mDoReconnect)
	{
		return;
	}
	mReconnectTimer.cancel();
	// try to reconnect, then call handle_connect
#ifdef WWW_ENABLE_SSL
	if (mSecure) {
		mSslSocket.lowest_layer().async_connect(m_EndPoint,
			boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
	}
	else
#endif
	{
		m_Socket.async_connect(m_EndPoint,
			boost::bind(&ASyncTCP::handle_connect, this, boost::asio::placeholders::error));
	}
	mIsReconnecting = false;
}

void ASyncTCP::do_write(const std::string &msg)
{
	if(!mIsConnected) return;

	if (!mIsClosing)
	{
		m_MsgBuffer = msg;
#ifdef WWW_ENABLE_SSL
		if (mSecure) {
			boost::asio::async_write(mSslSocket,
				boost::asio::buffer(m_MsgBuffer.c_str(), m_MsgBuffer.size()),
				boost::bind(&ASyncTCP::write_end, this, boost::asio::placeholders::error));
		}
		else
#endif
		{
			boost::asio::async_write(m_Socket,
				boost::asio::buffer(m_MsgBuffer.c_str(), m_MsgBuffer.size()),
				boost::bind(&ASyncTCP::write_end, this, boost::asio::placeholders::error));
		}
	}
}

/*
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
*/
