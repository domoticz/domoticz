#pragma once

#include "DomoticzHardware.h"
#include <list>
#include <mutex>
#include "XiaomiDeviceSupport.h"

#define MAX_LOG_LINE_LENGTH (2048 * 3)

class XiaomiGateway : public CDomoticzHardwareBase
{
      public:
	explicit XiaomiGateway(int ID);
	~XiaomiGateway();
	bool WriteToHardware(const char *pdata, unsigned char length) override;

	int GetGatewayHardwareID()
	{
		return m_HwdID;
	};
	std::string GetGatewayIp()
	{
		return m_GatewayIp;
	};
	std::string GetGatewaySid()
	{
		if (m_GatewaySID.empty())
			m_GatewaySID = XiaomiGatewayTokenManager::GetInstance().GetSID(m_GatewayIp);
		return m_GatewaySID;
	};

	bool IsMainGateway()
	{
		return m_ListenPort9898;
	};
	void SetAsMainGateway()
	{
		m_ListenPort9898 = true;
	};
	void UnSetMainGateway()
	{
		m_ListenPort9898 = false;
	};
	XiaomiDeviceSupport *GetDeviceSupport()
	{
		return m_DeviceSupport;
	};

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool SendMessageToGateway(const std::string &controlmessage);
	void InsertUpdateSwitch(const std::string &nodeid, const std::string &Name, bool bIsOn, _eSwitchType switchtype, int unittype, int level, const std::string &messagetype,
				const std::string &load_power, const std::string &power_consumed, int battery);
	void InsertUpdateRGBGateway(const std::string &nodeid, const std::string &Name, bool bIsOn, int brightness, int hue);
	void InsertUpdateCubeText(const std::string &nodeid, const std::string &Name, const std::string &degrees);
	void InsertUpdateVoltage(const std::string &nodeid, const std::string &Name, int VoltageLevel);
	void InsertUpdateLux(const std::string &nodeid, const std::string &Name, int Illumination, int battery);

	void InsertUpdateTemperature(const std::string &nodeid, const std::string &Name, float Temperature, int battery);
	void InsertUpdateHumidity(const std::string &nodeid, const std::string &Name, int Humidity, int battery);
	void InsertUpdatePressure(const std::string &nodeid, const std::string &Name, float Pressure, int battery);
	void InsertUpdateTempHumPressure(const std::string &nodeid, const std::string &Name, float Temperature, int Humidity, float Pressure, int battery);
	void InsertUpdateTempHum(const std::string &nodeid, const std::string &Name, float Temperature, int Humidity, int battery);

	std::string GetGatewayKey();
	unsigned int GetShortID(const std::string &nodeid);

	bool m_bDoRestart;
	std::shared_ptr<std::thread> m_thread;
	std::shared_ptr<std::thread> m_udp_thread;
	bool m_OutputMessage;
	bool m_IncludeVoltage;
	bool m_ListenPort9898;
	uint8_t m_GatewayRgbR;		// TODO: Remove, otherwise colors will be mixed up if controlling more than one bulb
	uint8_t m_GatewayRgbG;		// TODO: Remove, otherwise colors will be mixed up if controlling more than one bulb
	uint8_t m_GatewayRgbB;		// TODO: Remove, otherwise colors will be mixed up if controlling more than one bulb
	uint8_t m_GatewayBrightnessInt; // TODO: Remove, otherwise colors will be mixed up if controlling more than one bulb
	std::string m_GatewaySID;
	std::string m_GatewayIp;
	std::string m_LocalIp;
	std::string m_GatewayPassword;
	std::string m_GatewayMusicId;
	std::string m_GatewayVolume;
	std::mutex m_mutex;
	XiaomiDeviceSupport *m_DeviceSupport;

	XiaomiGateway *GatewayByIp(const std::string &ip);
	void AddGatewayToList();
	void RemoveFromGatewayList();

	int get_local_ipaddr(std::vector<std::string> &ip_addrs);

	std::string DetermineChannel(int32_t unitcode);
	std::string DetermineDevice(int32_t unitcode);
	std::string DetermineCommand(uint8_t commandcode);

	class xiaomi_udp_server
	{
	      public:
		xiaomi_udp_server(boost::asio::io_context &io_context, int m_HwdID, const std::string &gatewayIp, const std::string &localIp, bool listenPort9898, bool outputMessage,
				  bool includeVolage, XiaomiGateway *parent);
		~xiaomi_udp_server() = default;

	      private:
		void start_receive();
		void handle_receive(const boost::system::error_code &error, std::size_t /*bytes_transferred*/);

		boost::asio::ip::udp::socket socket_;
		boost::asio::ip::udp::endpoint remote_endpoint_;
		enum
		{
			max_length = 1024
		};
		char data_[max_length];
		int m_HardwareID;
		std::string m_gatewayip;
		std::string m_localip;
		bool m_OutputMessage;
		bool m_IncludeVoltage;
		XiaomiGateway *m_XiaomiGateway;
	};

	class XiaomiGatewayTokenManager
	{
	      public:
		static XiaomiGateway::XiaomiGatewayTokenManager &GetInstance();
		void UpdateTokenSID(const std::string &ip, const std::string &token, const std::string &sid);
		std::string GetToken(const std::string &ip);
		std::string GetSID(const std::string &sid);

	      private:
		std::mutex m_mutex;
		std::vector<std::tuple<std::string, std::string, std::string>> m_GatewayTokens;

		XiaomiGatewayTokenManager()
		{
			;
		}
		~XiaomiGatewayTokenManager()
		{
			;
		}
	};
};
