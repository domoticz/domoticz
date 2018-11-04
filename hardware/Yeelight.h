#pragma once

#include "DomoticzHardware.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class Yeelight : public CDomoticzHardwareBase
{
public:
	explicit Yeelight(const int ID);
	~Yeelight(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	void InsertUpdateSwitch(
		const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, const bool bIsOn, const std::string &yeelightBright, const std::string &yeelightHue,
		const std::string &yeelightSat, const std::string &yeelightRGB, const std::string &yeelightCT, const std::string &yeelightColorMode);
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
private:
	bool m_bDoRestart;
	std::shared_ptr<std::thread> m_thread;

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
