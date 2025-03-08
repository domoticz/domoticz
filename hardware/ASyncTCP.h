#pragma once

#include <stddef.h>			 // for size_t
#include <deque>			 // for write queue
#include <boost/asio/deadline_timer.hpp> // for deadline_timer
#include <boost/asio/io_context.hpp>	 // for io_context
#include <boost/asio/strand.hpp>	 // for strand
#include <boost/asio/ip/tcp.hpp>	 // for tcp, tcp::endpoint, tcp::s...
#include <boost/asio/ssl.hpp>		 // for secure sockets
#include <boost/asio/ssl/stream.hpp>	 // for secure sockets
#include <exception>			  // for exception
#include <optional>			// for optional

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

	boost::asio::io_context m_io_context; // protected to allow derived classes to attach timers etc.
private:
	void handle_resolve(const boost::system::error_code& ec, const boost::asio::ip::tcp::resolver::results_type &results);
	void handle_connect(const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint);
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

	void cb_write_queue(const std::string& msg);
	void do_write_start();
	void cb_write_done(const boost::system::error_code& error, size_t length);

	void process_connection();
	void process_error(const boost::system::error_code& error);

	bool m_bIsConnected = false;
	bool m_bIsReconnecting = false;
	bool m_bIsTerminating = false;

	boost::asio::io_context::strand m_SendStrand;
	std::deque<std::string> m_WriteQ; // we need a write queue to allow concurrent writes

	uint8_t* m_pRXBuffer = nullptr;

	int m_iReconnectDelay = DEFAULT_RECONNECT_TIME;
	int m_iTimeoutDelay = 0;
	boost::asio::deadline_timer m_ReconnectTimer;
	boost::asio::deadline_timer m_TimeoutTimer;

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
