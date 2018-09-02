#pragma once

#include <stddef.h>                        // for size_t
#include <boost/asio/deadline_timer.hpp>   // for deadline_timer
#include <boost/asio/io_service.hpp>       // for io_service
#include <boost/asio/ip/tcp.hpp>           // for tcp, tcp::endpoint, tcp::s...
#include <boost/function.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr
#include <exception>                       // for exception

namespace boost { namespace system { class error_code; } }

typedef std::shared_ptr<class ASyncTCP> ASyncTCPRef;

class ASyncTCP
{
public:
	ASyncTCP();
	virtual ~ASyncTCP(void);

	void connect(const std::string &ip, unsigned short port);
	void connect(boost::asio::ip::tcp::endpoint& endpoint);
	void disconnect();
	bool isConnected() { return mIsConnected; };
	void terminate(const bool silent = true);

	void write(const std::string &msg);
	void write(const uint8_t *pData, size_t length);

	void update();

	void SetReconnectDelay(int Delay = 30); //0 disabled retry

	/// Read complete callback
	boost::function<void()> callback_connect;
	boost::function<void()> callback_disconnect;
	boost::function<void(const unsigned char *pData, size_t length)> callback_data;
	boost::function<void(const std::exception e)> callback_error_std;
	boost::function<void(const boost::system::error_code& error)> callback_error_boost;
protected:
	void setCallbacks(
		const boost::function<void()>&cb_connect,
		const boost::function<void()>&cb_disconnect,
		const boost::function<void(const unsigned char *pData, size_t length)>&cb_data,
		const boost::function<void(const std::exception e)>&cb_error_std,
		const boost::function<void(const boost::system::error_code& error)>&cb_error_boost
	);
	void clearCallbacks();

	void read();
	void close();

	//bool set_tcp_keepalive();

	// callbacks
	void handle_connect(const boost::system::error_code& error);
	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
	void write_end(const boost::system::error_code& error);
	void do_close();

	void do_reconnect(const boost::system::error_code& error);
private:
	void OnErrorInt(const boost::system::error_code& error);
	void StartReconnect();
	int m_reconnect_delay;
protected:
	bool							mIsConnected;
	bool							mIsClosing;
	bool							mDoReconnect;
	bool							mIsReconnecting;

	boost::asio::ip::tcp::endpoint	mEndPoint;

	boost::asio::io_service			mIos;
	boost::asio::ip::tcp::socket	mSocket;

	unsigned char m_buffer[1024];

	boost::asio::deadline_timer		mReconnectTimer;

};
