#pragma once

#include <deque>
#include <iostream>
#include "MQTT.h"

class MySensorsMQTT : public MQTT
{
public:
	MySensorsMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename, const int Topics);
	~MySensorsMQTT(void);
public:
	void on_message(const struct mosquitto_message *message);
	void on_connect(int rc);
private:
	void ProcessMySensorsMessage(const std::string &MySensorsMessage);
protected:
	bool StartHardware();
	bool StopHardware();
	void SendHeartbeat();
};

