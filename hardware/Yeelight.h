#pragma once

#include "DomoticzHardware.h"
#include <boost/asio.hpp>

class Yeelight : public CDomoticzHardwareBase
{
      public:
	explicit Yeelight(int ID);
	~Yeelight() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	void InsertUpdateSwitch(const std::string &nodeID, const std::string &SketchName, const int &YeeType, const std::string &Location, bool bIsOn, const std::string &yeelightBright,
				const std::string &yeelightHue, const std::string &yeelightSat, const std::string &yeelightRGB, const std::string &yeelightCT, const std::string &yeelightColorMode);

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
		udp_server(boost::asio::io_context &io_context, int m_HwdID);
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		void start_send();

	      private:
		void start_receive();
		bool HandleIncoming(const std::string &szData, std::vector<std::string> &receivedip);
	};
};
