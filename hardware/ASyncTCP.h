#pragma once

#include <stddef.h>
#include <atomic>
#include <cstdint>
#include <deque>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#define ASYNCTCP_THREAD_NAME "ASyncTCP"
#define DEFAULT_RECONNECT_TIME 30
#define DEFAULT_TIMEOUT_TIME 60

class ASyncTCP
{
protected:
	ASyncTCP(bool secure = false);
	virtual ~ASyncTCP();
	void connect(const std::string& hostname, uint16_t port);
	void disconnect(bool silent = true);
	void write(const std::string& msg);
	void write(const uint8_t* pData, size_t length);
	void SetReconnectDelay(const int32_t Delay = DEFAULT_RECONNECT_TIME);
	bool isConnected()
	{
		return m_bIsConnected;
	};
	void terminate(bool silent = true);
	void SetTimeout(uint32_t Timeout = DEFAULT_TIMEOUT_TIME);

	// Callback interface to implement in derived classes
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnData(const uint8_t* pData, size_t length) = 0;
	virtual void OnError(const boost::system::error_code& error) = 0;

	// When a write fails, drop the failed frame instead of resending it after the reconnect. Default: keep it.
	virtual bool DropFailedWriteOnError() const { return false; }

	boost::asio::io_context m_io_context; // protected to allow derived classes to attach timers etc.
private:
	void handle_resolve(const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type &results, uint64_t generation);
	void handle_connect(const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint, uint64_t generation);
#ifdef WWW_ENABLE_SSL
	void cb_handshake_done(const boost::system::error_code& error);
#endif

	void timeout_start_timer();
	void timeout_cancel_timer();
	void reconnect_start_timer();
	void timeout_handler(const boost::system::error_code& error);

	void cb_reconnect_start(const boost::system::error_code& error);

	void do_close();

	void do_read_start();
	void cb_read_done(const boost::system::error_code& error, size_t bytes_transferred);

	void cb_write_queue(std::shared_ptr<const std::string> msg);
	void do_write_start();
	void cb_write_done(const boost::system::error_code& error, size_t length);

	void process_connection();
	void process_error(const boost::system::error_code& error);

	// Written by the caller's thread (terminate/disconnect) and the I/O thread.
	std::atomic<bool> m_bIsConnected{ false };
	std::atomic<bool> m_bIsReconnecting{ false };
	std::atomic<bool> m_bIsTerminating{ false };
	// Bumped by do_close() and terminate(). Handlers capture it when they are started and return early when it
	// changed, so a late completion of a closed session cannot act on the next one.
	std::atomic<uint64_t> m_ConnGeneration{ 0 };

	boost::asio::io_context::strand m_SendStrand;
	bool m_bWriteInFlight{ false }; // I/O thread only: m_WriteQ.front() is owned by a pending async_write
	// we need a write queue to allow concurrent writes. Entries are shared with the completion handler of the
	// pending write, so dropping the front entry in do_close() cannot free a buffer that is still in use.
	std::deque<std::shared_ptr<const std::string>> m_WriteQ;

	uint8_t* m_pRXBuffer = nullptr;

	int m_iReconnectDelay = DEFAULT_RECONNECT_TIME;
	int m_iTimeoutDelay = 0;
	boost::asio::steady_timer m_ReconnectTimer;
	boost::asio::steady_timer m_TimeoutTimer;

	std::shared_ptr<std::thread> m_Tcpthread;
	std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_Tcpwork;

#ifdef WWW_ENABLE_SSL
	const bool m_bSecure;
	boost::asio::ssl::context mContext{ boost::asio::ssl::context::sslv23 };
	std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> m_SslSocket;
#endif
	boost::asio::ip::tcp::socket m_Socket;
	boost::asio::ip::tcp::resolver m_Resolver;

	std::string m_IP;
	uint16_t m_Port;
};
