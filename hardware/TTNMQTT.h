#pragma once

#include "MySensorsBase.h"
#include "../main/mosquitto_helper.h"
#include "../main/BaroForecastCalculator.h"

namespace Json
{
	class Value;
} // namespace Json

class CTTNMQTT : public MySensorsBase, mosqdz::mosquittodz
{
      public:
	CTTNMQTT(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAFile);
	~CTTNMQTT() override;
	bool isConnected()
	{
		return m_IsConnected;
	};

	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

	void SendMessage(const std::string &Topic, const std::string &Message);

      public:
	void UpdateUserVariable(const std::string &varName, const std::string &varValue);

	bool m_bDoReconnect;
	bool m_IsConnected;
	boost::signals2::signal<void()> sDisconnected;

      protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	std::string m_TopicIn;
	std::shared_ptr<std::thread> m_thread;
	double m_DomLat;
	double m_DomLon;

	bool StartHardware() override;
	bool StopHardware() override;
	void StopMQTT();
	void Do_Work();
	virtual void SendHeartbeat();
	void WriteInt(const std::string &sendStr) override;

      private:
	std::map<std::string, CBaroForecastCalculator> m_forecast_calculators;

	bool ConnectInt();
	bool ConnectIntEx();
	Json::Value GetSensorWithChannel(const Json::Value &root, uint8_t sChannel);
	void FlagSensorWithChannelUsed(Json::Value &root, const std::string &stype, uint8_t sChannel);
	bool ConvertField2Payload(const std::string &sType, const std::string &sValue, uint8_t channel, uint8_t index, Json::Value &payload);
	bool ConvertFields2Payload(const Json::Value &fields, Json::Value &payload);
	int CalcDomoticsRssiFromLora(int gwrssi, float gwsnr);
	int GetAddDeviceAndSensor(int m_HwdId, const std::string &DeviceName, const std::string &MacAddress);
};
