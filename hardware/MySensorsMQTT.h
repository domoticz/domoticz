#pragma once

#include "MQTT.h"

class MySensorsMQTT : public MQTT
{
public:
	MySensorsMQTT(const int ID, const std::string &Name, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename, const int TLS_Version, const int Topics);
	~MySensorsMQTT(void);
public:
	void on_message(const struct mosquitto_message *message) override;
	void on_connect(int rc) override;
private:
	std::string ConvertMessageToMySensorsLine(const std::string &topic, const std::string &qMessage);
	void ConvertMySensorsLineToMessage(const std::string &sendStr, std::string &sTopic, std::string &sPayload);
	bool StartHardware() override;
	bool StopHardware() override;
	void SendHeartbeat() override;
	void WriteInt(const std::string &sendStr) override;
private:
	std::string m_TopicInWithoutHash;
	std::string MyTopicIn;
	std::string MyTopicOut;
};

