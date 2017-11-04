#include "stdafx.h"
#include "MQTT.h"
#include "MySensorsMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define TOPIC_DEFAULT	"MyMQTT"
#define TOPIC_IN		"domoticz/in/"
#define TOPIC_OUT		"domoticz/out/"

MySensorsMQTT::MySensorsMQTT(const int ID, const std::string &Name, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename, const int Topics) :
	MQTT(ID, IPAddress, usIPPort, Username, Password, CAfilename, (int)MQTT::PT_out),
	MyTopicIn(TOPIC_IN),
	MyTopicOut(TOPIC_OUT)
{
	switch (Topics) {
		case 1:
			MyTopicIn += Name;
			MyTopicOut += Name;
			break;
		case 0:
		default:
			MyTopicIn += TOPIC_DEFAULT;
			MyTopicOut += TOPIC_DEFAULT;
			break;
	}
	m_TopicInWithoutHash = MyTopicIn;
	m_TopicIn = m_TopicInWithoutHash + "/#";
	m_TopicOut = MyTopicOut;

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

	if (topic.empty() && qMessage.empty())
		return;

	std::string sMessage = ConvertMessageToMySensorsLine(topic, qMessage);
	ProcessMySensorsMessage(sMessage);
}

std::string MySensorsMQTT::ConvertMessageToMySensorsLine(const std::string &topic, const std::string &qMessage)
{
	std::string sMessage = topic + "/" + qMessage;
	boost::replace_all(sMessage, m_TopicInWithoutHash, "");
	boost::replace_all(sMessage, "/", ";");
	if (sMessage[0] == ';')
	{
		sMessage = sMessage.substr(1);
	}

	return sMessage;
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

void MySensorsMQTT::WriteInt(const std::string &sendStr)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);

	std::string sTopic;
	std::string sPayload;
	ConvertMySensorsLineToMessage(sendStr, sTopic, sPayload);

	SendMessage(sTopic, sPayload);
}

void MySensorsMQTT::ConvertMySensorsLineToMessage(const std::string &sLine, std::string &sTopic, std::string &sPayload)
{
	if (sLine.size() < 2)
	{
		return;
	}

	size_t indexLastSeperator = sLine.find_last_of(';');
	if (indexLastSeperator == std::string::npos)
	{
		return;
	}

	sTopic = std::string(sLine.substr(0, indexLastSeperator).c_str());
	boost::replace_all(sTopic, ";", "/");
	sTopic.insert(0, m_TopicOut + "/");

	sPayload = std::string(sLine.substr(indexLastSeperator + 1).c_str());
	if (!sPayload.empty() &&
		sPayload[sPayload.length() - 1] == '\n')
	{
		sPayload.resize(sPayload.length() - 1);
	}
}
