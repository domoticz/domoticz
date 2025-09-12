#include "stdafx.h"
#include "ASyncTCP.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "../main/Logger.h"

#define MAX_TCP_BUFFER_SIZE 4096

#define STATUS_OK(err) !err
#define STATUS_ERR(err) err

ASyncTCP::ASyncTCP(const bool secure) :
	m_Tcpwork(boost::asio::make_work_guard(m_io_context))
	, m_Socket(m_io_context)
	, m_Resolver(m_io_context)
	, m_ReconnectTimer(m_io_context)
	, m_TimeoutTimer(m_io_context)
	, m_SendStrand(m_io_context)
#ifdef WWW_ENABLE_SSL
	, m_bSecure(secure)
#endif
{
	m_pRXBuffer = new uint8_t[MAX_TCP_BUFFER_SIZE];
#ifdef WWW_ENABLE_SSL
	mContext.set_verify_mode(boost::asio::ssl::verify_none);
	if (m_bSecure)
	{
		m_SslSocket.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(m_io_context, mContext));
	}
#endif
}

ASyncTCP::~ASyncTCP()
{
	assert(m_Tcpthread == nullptr);
	m_bIsTerminating = true;
	if (m_Tcpthread)
	{
		//This should never happen. terminate() never called!!
		_log.Log(LOG_ERROR, "ASyncTCP: Worker thread not closed. terminate() never called!!!");
		m_io_context.stop();
		if (m_Tcpthread)
		{
			m_Tcpthread->join();
			m_Tcpthread.reset();
		}
	}
	if (m_pRXBuffer != nullptr)
		delete[] m_pRXBuffer;
}

void ASyncTCP::SetReconnectDelay(const int32_t Delay)
{
	m_iReconnectDelay = Delay;
}

void ASyncTCP::connect(const std::string& ip, uint16_t port)
{
	assert(!m_Socket.is_open());
	if (m_Socket.is_open())
	{
		_log.Log(LOG_ERROR, "ASyncTCP: connect called while socket is still open. !!!");
		terminate();
	}

	m_IP = ip;
	m_Port = port;
	std::string port_str = std::to_string(port);
	timeout_start_timer();

	m_Resolver.async_resolve(
		ip, port_str,
		[this](const boost::system::error_code& error, const boost::asio::ip::tcp::resolver::results_type& endpoints) {
			handle_resolve(error, endpoints);
		}
	);

	// RK: We restart m_io_context here because it might have been stopped in terminate()
	m_io_context.restart();
	// RK: After the reset, we need to provide it work anew
	m_Tcpwork.reset();
	m_Tcpwork.emplace(boost::asio::make_work_guard(m_io_context));
	if (!m_Tcpthread)
		m_Tcpthread = std::make_shared<std::thread>([p = &m_io_context] { p->run(); });
}

void ASyncTCP::handle_resolve(const boost::system::error_code& error, const boost::asio::ip::tcp::resolver::results_type &endpoints)
{
	if (m_bIsTerminating) return;

	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
	if (m_bIsConnected) return;

	timeout_start_timer();

#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		// we reset the ssl socket, because the ssl context needs to be reinitialized after a reconnect
		m_SslSocket.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(m_io_context, mContext));
		boost::asio::async_connect(m_SslSocket->lowest_layer(), endpoints,
			[this](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
			{
				handle_connect(error, endpoint);
			}
		);
	}
	else
#endif
	{
		boost::asio::async_connect(m_Socket, endpoints,
			[this](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
			{
				handle_connect(error, endpoint);
			}
		);
	}
}

void ASyncTCP::handle_connect(const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& /*endpoint*/)
{
	if (m_bIsTerminating) return;

	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		timeout_start_timer();
		m_SslSocket->async_handshake(boost::asio::ssl::stream_base::client,
			[this](const boost::system::error_code& error) {
				cb_handshake_done(error);
			}
		);
	}
	else
#endif
	{
		process_connection();
	}
}

#ifdef WWW_ENABLE_SSL
void ASyncTCP::cb_handshake_done(const boost::system::error_code& error)
{
	if (m_bIsTerminating) return;

	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
	process_connection();
#endif
}

void ASyncTCP::process_connection()
{
	m_bIsConnected = true;
#ifdef WWW_ENABLE_SSL

	if (!m_bSecure)
#endif
	{
		// RK: only if non-secure
		boost::asio::socket_base::keep_alive option(true);
		m_Socket.set_option(option);
	}
	OnConnect();
	do_read_start();
	do_write_start();
}

void ASyncTCP::reconnect_start_timer()
{
	if (m_bIsReconnecting) return;

	if (m_iReconnectDelay != 0)
	{
		m_bIsReconnecting = true;

		m_ReconnectTimer.expires_from_now(boost::posix_time::seconds(m_iReconnectDelay));
		m_ReconnectTimer.async_wait(
			[this](const boost::system::error_code& error) {
				cb_reconnect_start(error);
			}
		);
	}
}

void ASyncTCP::cb_reconnect_start(const boost::system::error_code& error)
{
	m_bIsReconnecting = false;
	m_ReconnectTimer.cancel();
	m_TimeoutTimer.cancel();

	if (m_bIsConnected) return;
	if (error) return; // timer was cancelled

	do_close();
	connect(m_IP, m_Port);
}


void ASyncTCP::terminate(const bool silent)
{
	m_bIsTerminating = true;
	disconnect(silent);
	m_Tcpwork.reset();
	m_io_context.stop();
	if (m_Tcpthread)
	{
		m_Tcpthread->join();
		m_Tcpthread.reset();
	}
	m_bIsReconnecting = false;
	m_bIsConnected = false;
	m_WriteQ.clear();
	m_bIsTerminating = false;
}

void ASyncTCP::disconnect(const bool silent)
{
	m_ReconnectTimer.cancel();
	m_TimeoutTimer.cancel();
	if (!m_Tcpthread) return;

	try
	{
		boost::asio::post(m_io_context, 
			[this] {
				do_close();
			}
		);
	}
	catch (...)
	{
		if (silent == false)
		{
			throw;
		}
	}
}

void ASyncTCP::do_close()
{
	if (m_bIsReconnecting) {
		return;
	}
	m_ReconnectTimer.cancel();
	m_TimeoutTimer.cancel();
	boost::system::error_code ec;
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		if (m_SslSocket->lowest_layer().is_open())
		{
			m_SslSocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			m_SslSocket->lowest_layer().close(ec);
		}
	}
	else
#endif
	{
		if (m_Socket.is_open())
		{
			m_Socket.close(ec);
		}
	}
}

void ASyncTCP::do_read_start()
{
	if (m_bIsTerminating) return;
	if (!m_bIsConnected) return;

	timeout_start_timer();
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		m_SslSocket->async_read_some(boost::asio::buffer(m_pRXBuffer, MAX_TCP_BUFFER_SIZE),
			[this](const boost::system::error_code& error, size_t bytes_transferred) {
				cb_read_done(error, bytes_transferred);
			}
		);
	}
	else
#endif
	{
		m_Socket.async_read_some(boost::asio::buffer(m_pRXBuffer, MAX_TCP_BUFFER_SIZE),
			[this](const boost::system::error_code& error, size_t bytes_transferred) {
				cb_read_done(error, bytes_transferred);
			}
		);
	}
}

void ASyncTCP::cb_read_done(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (m_bIsTerminating) return;

	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
	OnData(m_pRXBuffer, bytes_transferred);
	do_read_start();
}

void ASyncTCP::write(const uint8_t* pData, size_t length)
{
	write(std::string((const char*)pData, length));
}

void ASyncTCP::write(const std::string& msg)
{
	if (!m_Tcpthread) return;

	boost::asio::post(m_SendStrand, [this, msg]() { cb_write_queue(msg); });
}

void ASyncTCP::cb_write_queue(const std::string& msg)
{
	m_WriteQ.push_back(msg);

	if (m_WriteQ.size() == 1)
		do_write_start();
}

void ASyncTCP::do_write_start()
{
	if (m_bIsTerminating) return;
	if (!m_bIsConnected) return;
	if (m_WriteQ.empty())
		return;

	timeout_start_timer();
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		boost::asio::async_write(*m_SslSocket, boost::asio::buffer(m_WriteQ.front()),
			[this](const boost::system::error_code& error, std::size_t length) {
				cb_write_done(error, length);
			}
		);
	}
	else
#endif
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(m_WriteQ.front()),
			[this](const boost::system::error_code& error, std::size_t length) {
				cb_write_done(error, length);
			}
		);
	}
}

void ASyncTCP::cb_write_done(const boost::system::error_code& error, std::size_t /*length*/)
{
	if (m_bIsTerminating) return;

	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
	m_WriteQ.pop_front();
	do_write_start();
}

void ASyncTCP::process_error(const boost::system::error_code& error)
{
	do_close();
	if (m_bIsConnected)
	{
		m_bIsConnected = false;
		OnDisconnect();
	}

	if (boost::asio::error::operation_aborted == error)
		return;

	OnError(error);
	reconnect_start_timer();
}

void ASyncTCP::timeout_start_timer()
{
	if (0 == m_iTimeoutDelay) {
		return;
	}
	timeout_cancel_timer();
	m_TimeoutTimer.expires_from_now(boost::posix_time::seconds(m_iTimeoutDelay));
	m_TimeoutTimer.async_wait(
		[this](const boost::system::error_code& error) {
			timeout_handler(error);
		}
	);
}

void ASyncTCP::timeout_cancel_timer()
{
	m_TimeoutTimer.cancel();
}

void ASyncTCP::timeout_handler(const boost::system::error_code& error)
{
	if (error) {
		// timer was cancelled on time
		return;
	}
	boost::system::error_code err = make_error_code(boost::system::errc::timed_out);
	process_error(err);
}

void ASyncTCP::SetTimeout(const uint32_t Timeout)
{
	m_iTimeoutDelay = Timeout;
}
