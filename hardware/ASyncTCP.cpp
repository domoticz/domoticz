#include "stdafx.h"
#include "ASyncTCP.h"
#include <cassert>
#include <exception>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include "../main/Logger.h"

#define MAX_TCP_BUFFER_SIZE 4096

#define STATUS_OK(err) !err
#define STATUS_ERR(err) err

ASyncTCP::ASyncTCP(const bool secure) :
	m_SendStrand(m_io_context)
	, m_ReconnectTimer(m_io_context)
	, m_TimeoutTimer(m_io_context)
	, m_Tcpwork(boost::asio::make_work_guard(m_io_context))
#ifdef WWW_ENABLE_SSL
	, m_bSecure(secure)
#endif
	, m_Socket(m_io_context)
	, m_Resolver(m_io_context)
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
	m_bIsTerminating = true;
	if (m_Tcpthread)
	{
		//This should never happen. terminate() never called!!
		_log.Log(LOG_ERROR, "ASyncTCP: Worker thread not closed. terminate() never called!!!");
		// Precondition: the owner is not destroyed from one of its own handlers. Joining the I/O thread from itself
		// deadlocks, and detaching it would let run() unwind over a destroyed io_context, so that case is fatal.
		assert(m_Tcpthread->get_id() != std::this_thread::get_id());
		if (m_Tcpthread->get_id() == std::this_thread::get_id())
		{
			_log.Log(LOG_ERROR, "ASyncTCP: destroyed from its own I/O thread, cannot continue safely");
			std::terminate();
		}
		m_io_context.stop();
		m_Tcpthread->join();
		m_Tcpthread.reset();
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

	const uint64_t generation = m_ConnGeneration;
	m_Resolver.async_resolve(
		ip, port_str,
		[this, generation](const boost::system::error_code& error, const boost::asio::ip::tcp::resolver::results_type& endpoints) {
			if (generation != m_ConnGeneration) return; // belongs to a session that was closed meanwhile
			handle_resolve(error, endpoints, generation);
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

void ASyncTCP::handle_resolve(const boost::system::error_code& error, const boost::asio::ip::tcp::resolver::results_type &endpoints, const uint64_t generation)
{
	if (m_bIsTerminating) return;
	if (error == boost::asio::error::operation_aborted) return; // cancelled by do_close() or terminate()

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
			[this, generation](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
			{
				if (generation != m_ConnGeneration) return;
				handle_connect(error, endpoint, generation);
			}
		);
	}
	else
#endif
	{
		boost::asio::async_connect(m_Socket, endpoints,
			[this, generation](const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint)
			{
				if (generation != m_ConnGeneration) return;
				handle_connect(error, endpoint, generation);
			}
		);
	}
}

void ASyncTCP::handle_connect(const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& /*endpoint*/, const uint64_t generation)
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
			[this, generation](const boost::system::error_code& error) {
				if (generation != m_ConnGeneration) return;
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
}
#endif

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

		m_ReconnectTimer.expires_after(std::chrono::seconds(m_iReconnectDelay));
		m_ReconnectTimer.async_wait(
			[this, generation = m_ConnGeneration.load()](const boost::system::error_code& error) {
				if (generation != m_ConnGeneration)
				{
					m_bIsReconnecting = false; // a later close must not be blocked by this stale timer
					return;
				}
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

	if (m_bIsTerminating) return;
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

	// The I/O thread is gone, so everything below runs single threaded. stop() leaves queued handlers behind and
	// they would otherwise run on the next connect(). Cancel what is pending, close the socket directly (a posted
	// close may never have run) and drain the handlers while m_bIsTerminating is still set, so they return early.
	m_ConnGeneration++;
	m_ReconnectTimer.cancel();
	m_TimeoutTimer.cancel();
	m_Resolver.cancel();
	do_close();
	m_io_context.restart();
	m_io_context.poll();
	m_io_context.stop();

	m_WriteQ.clear();
	m_bIsTerminating = false;
}

void ASyncTCP::disconnect(const bool silent)
{
	m_bIsConnected = false;
	if (!m_Tcpthread) return;

	try
	{
		boost::asio::post(m_io_context,
			[this] {
				m_ReconnectTimer.cancel();
				m_bIsReconnecting = false; // the cancelled timer's handler is generation guarded, so clear it here
				m_TimeoutTimer.cancel();
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
	if (m_bWriteInFlight)
	{
		// A close that cuts a write short (timeout, read error, disconnect) abandons the pending write: drop its
		// entry so a partially sent frame is not resent after a reconnect. A write that failed by itself was
		// already handled by cb_write_done().
		if (!m_WriteQ.empty())
			m_WriteQ.pop_front();
		m_bWriteInFlight = false;
	}
	// Frames that were queued but not yet written survive a close by design: users such as DaikinModbus and
	// EvohomeTCP write while offline and rely on the replay after a reconnect. Owners that must not replay
	// (Shelly, the RFLink restart path) clear the queue through terminate().
	m_ReconnectTimer.cancel();
	m_TimeoutTimer.cancel();
	m_Resolver.cancel(); // a late resolve must not start a connect on a socket that was just closed
	m_ConnGeneration++; // completions of the session being closed must not act on the next one
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

	const uint64_t generation = m_ConnGeneration;
	timeout_start_timer();
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		m_SslSocket->async_read_some(boost::asio::buffer(m_pRXBuffer, MAX_TCP_BUFFER_SIZE),
			[this, generation](const boost::system::error_code& error, size_t bytes_transferred) {
				if (generation != m_ConnGeneration) return;
				cb_read_done(error, bytes_transferred);
			}
		);
	}
	else
#endif
	{
		m_Socket.async_read_some(boost::asio::buffer(m_pRXBuffer, MAX_TCP_BUFFER_SIZE),
			[this, generation](const boost::system::error_code& error, size_t bytes_transferred) {
				if (generation != m_ConnGeneration) return;
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

	auto shared = std::make_shared<const std::string>(msg);
	boost::asio::post(m_SendStrand, [this, shared = std::move(shared)]() mutable { cb_write_queue(std::move(shared)); });
}

void ASyncTCP::cb_write_queue(std::shared_ptr<const std::string> msg)
{
	m_WriteQ.push_back(std::move(msg));

	if (m_WriteQ.size() == 1)
		do_write_start();
}

void ASyncTCP::do_write_start()
{
	if (m_bIsTerminating) return;
	if (!m_bIsConnected) return;
	if (m_WriteQ.empty())
		return;

	const uint64_t generation = m_ConnGeneration;
	timeout_start_timer();
	m_bWriteInFlight = true;
	// The completion handler shares ownership of the message: do_close() may drop the queue entry while the
	// operation is still pending, and the buffer must stay valid until the operation has completed or was aborted.
	const std::shared_ptr<const std::string> msg = m_WriteQ.front();
#ifdef WWW_ENABLE_SSL
	if (m_bSecure)
	{
		boost::asio::async_write(*m_SslSocket, boost::asio::buffer(*msg),
			[this, generation, msg](const boost::system::error_code& error, std::size_t length) {
				if (generation != m_ConnGeneration) return;
				cb_write_done(error, length);
			}
		);
	}
	else
#endif
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(*msg),
			[this, generation, msg](const boost::system::error_code& error, std::size_t length) {
				if (generation != m_ConnGeneration) return;
				cb_write_done(error, length);
			}
		);
	}
}

void ASyncTCP::cb_write_done(const boost::system::error_code& error, std::size_t /*length*/)
{
	m_bWriteInFlight = false;
	if (m_bIsTerminating) return;

	// A successful write finishes the entry. After an error the stream position is unknown: a framed protocol
	// (websocket) must not replay the frame on the next connection, so such subclasses drop it; other users keep
	// the frame queued and it is resent after the reconnect. m_bWriteInFlight is already clear, so do_close()
	// below does not pop a second entry.
	if (!STATUS_ERR(error) || DropFailedWriteOnError())
	{
		if (!m_WriteQ.empty())
			m_WriteQ.pop_front();
	}
	if (STATUS_ERR(error))
	{
		process_error(error);
		return;
	}
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
	m_TimeoutTimer.expires_after(std::chrono::seconds(m_iTimeoutDelay));
	m_TimeoutTimer.async_wait(
		[this, generation = m_ConnGeneration.load()](const boost::system::error_code& error) {
			if (generation != m_ConnGeneration) return;
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
	if (error || m_bIsTerminating) {
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
