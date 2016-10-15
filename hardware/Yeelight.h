#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/deadline_timer.hpp>

class Yeelight : public CDomoticzHardwareBase
{
public:
	Yeelight(const int ID);
	~Yeelight(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue);
	void StopSocket();

public:
	//signals
	boost::signals2::signal<void()> sDisconnected;

private:
	bool StartHardware();
	bool StopHardware();

protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	class udp_server
	{
	public:
		udp_server(boost::asio::io_service & io_service, int m_HwdID);
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;

		bool stopped_;
		//boost::asio::streambuf input_buffer_;
		boost::asio::deadline_timer deadline_;


	private:
		void start_send();
		void start_receive();
		void handle_receive(const boost::system::error_code & error, std::size_t);
		void handle_send(boost::shared_ptr<std::string>, const boost::system::error_code &, std::size_t);
		void stop();
		void check_deadline();

	};


};

