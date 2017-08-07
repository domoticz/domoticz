#pragma once

#include "DomoticzHardware.h"
#include <deque>
#include <iostream>

class XiaomiGateway : public CDomoticzHardwareBase
{
public:
	XiaomiGateway(const int ID);
	~XiaomiGateway(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	bool SendMessageToGateway(const std::string &controlmessage);
	void InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, const bool bIsOn, const _eSwitchType switchtype, const int level, const std::string &messagetype, const bool isctlr2, const bool is2ndchannel, const std::string &load_power, const std::string &power_consumed, const int battery);
	void InsertUpdateCubeText(const std::string &nodeid, const std::string &Name, const std::string &degrees);
	void InsertUpdateVoltage(const std::string &nodeid, const std::string &Name, const int VoltageLevel);
	void InsertUpdateLux(const std::string &nodeid, const std::string &Name, const int Illumination);
	void InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, const float Temperature, const int battery);
	void InsertUpdateHumidity(const std::string &nodeid, const std::string &Name, const int Humidity, const int battery);
	void InsertUpdatePressure(const std::string &nodeid, const std::string &Name, const int Pressure, const int battery);
	void InsertUpdateRGBGateway(const std::string &nodeid, const std::string &Name, const bool bIsOn, const int brightness, const int hue);
	void UpdateToken(const std::string &value);

private:
	bool StartHardware();
	bool StopHardware();
	bool m_bDoRestart;
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	boost::shared_ptr<boost::thread> m_udp_thread;
	bool m_OutputMessage;
	bool m_ListenPort9898;
	std::string GetGatewayKey();
	unsigned int GetShortID(const std::string & nodeid);
	std::string m_GatewayRgbHex;
	int m_GatewayBrightnessInt;
	std::string m_GatewayPrefix;
	std::string m_GatewayIp;
	std::string m_LocalIp;
	std::string m_GatewayPassword;
	std::string m_token;
	std::string m_GatewayMusicId;
	std::string m_GatewayVolume;
	boost::mutex m_mutex;

	volatile bool m_stoprequested;

	class xiaomi_udp_server
	{
	public:
		xiaomi_udp_server(boost::asio::io_service & io_service, int m_HwdID, const std::string gatewayIp, const std::string localIp, const bool listenPort9898, const bool outputMessage, XiaomiGateway *parent);
		~xiaomi_udp_server();

	private:
		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		enum { max_length = 1024 };
		char data_[max_length];
		int m_HardwareID;
		std::string m_gatewayip;
		std::string m_localip;
		bool m_OutputMessage;
		XiaomiGateway* m_XiaomiGateway;
		void start_receive();
		void handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/);
	};
};
