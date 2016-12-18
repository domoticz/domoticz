#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class XiaomiGateway : public CDomoticzHardwareBase
{
public:
	XiaomiGateway(const int ID);
	~XiaomiGateway(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn, _eSwitchType type);

private:
	bool StartHardware();
	bool StopHardware();
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	boost::shared_ptr<boost::thread> m_udp_thread;
	volatile bool m_stoprequested;

	class xiaomi_udp_server
	{
	public :
		xiaomi_udp_server(boost::asio::io_service & io_service, int m_HwdID);
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
	private:
		enum { max_length = 1024 };
		char data_[max_length];
		//boost::array<char, 1024> recv_buffer_;
		int m_HardwareID;
		void start_receive();
		void handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
	};
};
