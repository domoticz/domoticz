#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iosfwd>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class Yeelight : public CDomoticzHardwareBase
{
public:
	Yeelight(const int ID);
	~Yeelight(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue);

public:
	//signals
	boost::signals2::signal<void()> sDisconnected;

private:
	bool StartHardware();
	bool StopHardware();
protected:
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
		void start_send();

	private:
		void start_receive();
		bool HandleIncoming(const std::string &szData, std::vector<std::string> &receivedip);
	};

};
