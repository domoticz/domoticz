#pragma once

#include "MySensorsBase.h"
#ifdef BUILTIN_MQTT
#include "../MQTT/mosquittopp.h"
#else
#ifdef WIN32
#include "../MQTT/mosquittopp.h"
#else
#include <mosquittopp.h>
#endif
#endif
#include "../main/BaroForecastCalculator.h"

namespace Json
{
	class Value;
};

class CTTNMQTT : public MySensorsBase, mosqpp::mosquittopp
{
public:
	CTTNMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAFile);
	~CTTNMQTT(void);
	bool isConnected(){ return m_IsConnected; };

	virtual void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	virtual void on_message(const struct mosquitto_message *message) override;
	void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

	void SendMessage(const std::string &Topic, const std::string &Message);

public:
	void UpdateUserVariable(const std::string &varName, const std::string &varValue);

	bool m_bDoReconnect;
	bool m_IsConnected;
	boost::signals2::signal<void()>	sDisconnected;
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_UserName;
	std::string m_Password;
	std::string m_CAFilename;
	std::string m_TopicIn;
	virtual bool StartHardware() override;
	virtual bool StopHardware() override;
	void StopMQTT();
	void Do_Work();
	virtual void SendHeartbeat();
	void WriteInt(const std::string &sendStr) override;
	std::shared_ptr<std::thread> m_thread;
private:
	bool ConnectInt();
	bool ConnectIntEx();
	Json::Value GetSensorWithChannel(const Json::Value &root, const std::string &stype, const int sChannel);
	void FlagSensorWithChannelUsed(Json::Value &root, const std::string &stype, const int sChannel);
	std::map<std::string, CBaroForecastCalculator> m_forecast_calculators;
};

