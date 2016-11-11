#include "stdafx.h"
#include "MySensorsMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

MySensorsMQTT::MySensorsMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename, const int Topics):
	MQTT(ID, IPAddress, usIPPort, Username, Password, CAfilename, Topics)
{
	m_TopicIn = "domoticz/in/MyMQTT";
	m_TopicOut = "domoticz/out/MyMQTT";
}

MySensorsMQTT::~MySensorsMQTT(void)
{
}

bool MySensorsMQTT::StartHardware()
{
	LoadDevicesFromDatabase();

	bool result = MQTT::StartHardware();
	StartSendQueue();

	return result;
}

bool MySensorsMQTT::StopHardware()
{
	StopSendQueue();
	return MQTT::StopHardware();
}

void MySensorsMQTT::on_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	_log.Log(LOG_NORM, "MySensorsMQTT: Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());

	if (qMessage.empty())
		return;

	ProcessMySensorsMessage(qMessage);
}

void MySensorsMQTT::ProcessMySensorsMessage(const std::string &MySensorsMessage)
{
	m_bufferpos = MySensorsMessage.size();
	memcpy(&m_buffer, MySensorsMessage.c_str(), m_bufferpos);
	m_buffer[m_bufferpos] = 0;
	ParseLine();
}

void MySensorsMQTT::on_connect(int rc)
{
	MQTT::on_connect(rc);

	if (m_IsConnected)
	{
		_log.Log(LOG_STATUS, "MySensorsMQTT: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		
		//Request gateway version
		std::string sRequest = "0;0;3;0;2;";
		WriteInt(sRequest);
	}
}

void MySensorsMQTT::SendHeartbeat()
{
	//Send a MySensors Heartbeat message to the gateway
	std::string sRequest = "0;0;3;0;18;PING";
	WriteInt(sRequest);
}