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

namespace boost
{
	namespace system
	{
		class error_code;
	} // namespace system
} // namespace boost

class ASyncTCP
{
      protected:
	ASyncTCP(bool secure = false);
	virtual ~ASyncTCP();

	void connect(const std::string &hostname, uint16_t port);
	void disconnect(bool silent = true);
	void write(const std::string &msg);
	void write(const uint8_t *pData, size_t length);
	void SetReconnectDelay(int32_t Delay = DEFAULT_RECONNECT_TIME);
	bool isConnected()
	{
		return mIsConnected;
	};
	void terminate(bool silent = true);
	void SetTimeout(uint32_t Timeout = DEFAULT_TIMEOUT_TIME);

	// Callback interface to implement in derived classes
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnData(const uint8_t *pData, size_t length) = 0;
	virtual void OnError(const boost::system::error_code &error) = 0;

	boost::asio::io_context mIoc; // protected to allow derived classes to attach timers etc.

      private:
	void cb_resolve_done(const boost::system::error_code &err, boost::asio::ip::tcp::resolver::results_type endpoints);
	void connect_start(boost::asio::ip::tcp::resolver::results_type &endpoints);
	void cb_connect_done(const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &endpoint);
#ifdef WWW_ENABLE_SSL
	void cb_handshake_done(const boost::system::error_code &error);
#endif

	/* timeout methods */
	void timeout_start_timer();
	void timeout_cancel_timer();
	void reconnect_start_timer();
	void timeout_handler(const boost::system::error_code &error);

	void cb_reconnect_start(const boost::system::error_code &error);

	void do_close();

	void do_read_start();
	void cb_read_done(const boost::system::error_code &error, size_t bytes_transferred);

	void cb_write_queue(const std::string &msg);
	void do_write_start();
	void cb_write_done(const boost::system::error_code &error);

	void process_connection();
	void process_error(const boost::system::error_code &error);

	bool mIsConnected = false;
	bool mIsReconnecting = false;
	bool mIsTerminating = false;

	boost::asio::io_context::strand mSendStrand{ mIoc };
	std::deque<std::string> mWriteQ; // we need a write queue to allow concurrent writes

	uint8_t* m_pRXBuffer = nullptr;

	int mReconnectDelay = DEFAULT_RECONNECT_TIME;
	int mTimeoutDelay = 0;
	boost::asio::deadline_timer mReconnectTimer{ mIoc };
	boost::asio::deadline_timer mTimeoutTimer{ mIoc };

	std::shared_ptr<std::thread> mTcpthread;
	std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> mTcpwork;

#ifdef WWW_ENABLE_SSL
	const bool mSecure;
	boost::asio::ssl::context mContext{ boost::asio::ssl::context::sslv23 };
	std::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> mSslSocket; // the ssl socket
#endif
	boost::asio::ip::tcp::socket mSocket{ mIoc };
	boost::asio::ip::tcp::resolver mResolver{ mIoc };

	std::string mIp;
	uint16_t mPort;
};
