#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>

class yeelightClient //: public CDomoticzHardwareBase
{
public:
	yeelightClient(boost::asio::io_service& io_service);
	~yeelightClient(void);
	void start(boost::asio::ip::tcp::resolver::iterator endpoint_iter);
	void stop();
private:
	void start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iter);
	void handle_connect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iter);
	void start_read();
	void handle_read(const boost::system::error_code& ec);
	void start_write();
	void handle_write(const boost::system::error_code& ec);
	void check_deadline();
private:
	bool stopped_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf input_buffer_;
	boost::asio::deadline_timer deadline_;
	boost::asio::deadline_timer heartbeat_timer_;
};