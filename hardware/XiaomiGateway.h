#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <openssl/aes.h>

class XiaomiGateway : public CDomoticzHardwareBase
{
public:
	XiaomiGateway(const int ID);
	~XiaomiGateway(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn, _eSwitchType subtype);
	void InsertUpdateVoltage(const std::string &nodeid, const std::string &Name, const int BatteryLevel);
	void InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, const float Temperature);
	void InsertUpdateHumidity(const std::string &nodeid, const std::string &Name, const int Humidity);
	void UpdateToken(const std::string &value);

private:
	bool StartHardware();
	bool StopHardware();
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	boost::shared_ptr<boost::thread> m_udp_thread;
	std::string GetGatewayKey();
	std::string m_GatewayIp;
	std::string m_GatewayPassword;
	std::string m_token;
	boost::mutex m_mutex;

	volatile bool m_stoprequested;

	class xiaomi_udp_server
	{
	public:
		xiaomi_udp_server(boost::asio::io_service & io_service, int m_HwdID, XiaomiGateway *parent);

	private:
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		enum { max_length = 512 };
		char data_[max_length];
		int m_HardwareID;
		std::string m_gatewayip;
		XiaomiGateway* m_XiaomiGateway;
		void start_receive();
		void handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
	};
};
