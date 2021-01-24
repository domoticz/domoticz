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

MySensorsMQTT::MySensorsMQTT(const int ID, const std::string &Name, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password,
			     const std::string &CAfilenameExtra, const int TLS_Version, const int PublishScheme, const bool PreventLoop)
	: MQTT(ID, IPAddress, usIPPort, Username, Password, CAfilenameExtra, TLS_Version, (int)MQTT::PT_out, std::string("Domoticz-MySensors") + std::string(GenerateUUID()), PreventLoop)
	, MyTopicIn(TOPIC_IN)
	, MyTopicOut(TOPIC_OUT)
{

	/**
	 *
	 * There's no way to know the state of the CAfilename storage, so always try to separate
	 * out the topic in/out prefixes.
	 *
	 **/

	size_t nextPiece = std::string::npos;
	std::string CustomTopicIn;
	std::string CustomTopicOut;

	do {
		// Locate the last delimiter in the CAfilename string.
		nextPiece = m_CAFilename.rfind('#');
		if (std::string::npos == nextPiece)
		{
			// No delimiter; It's just a CA filename.
			break;
		}

		// First delimiter present; Store the MyTopicOut prefix.
		CustomTopicOut = m_CAFilename.substr(nextPiece + 1, m_CAFilename.length());
		// And remove it from the CAfilename string.
		m_CAFilename.erase(nextPiece, m_CAFilename.length());

		// Locate the second to last delimiter in the CAfilename string.
		nextPiece = m_CAFilename.rfind('#');
		if (std::string::npos == nextPiece)
		{
			// No second to last delimiter? Shouldn't happen.
			_log.Log(LOG_ERROR, "MySensorsMQTT: Truncating CAfilename; Stray topic was present.");
			break;
		}

		// Second to last delimiter present; Store the MyTopicIn prefix.
		CustomTopicIn = m_CAFilename.substr(nextPiece + 1, m_CAFilename.length());
		// And remove it from the CAfilename string.
		m_CAFilename.erase(nextPiece, m_CAFilename.length());

	} while (false);

	switch (PublishScheme) {
		case 2:
			MyTopicIn = CustomTopicIn;
			MyTopicOut = CustomTopicOut;
			break;
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

bool MySensorsMQTT::StartHardware()
{
	RequestStart();

	m_LineReceived.clear();

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
	ParseLine(sMessage);
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

void MySensorsMQTT::on_connect(int rc)
{
	MQTT::on_connect(rc);

	if (m_IsConnected)
	{
		_log.Log(LOG_STATUS, "MySensorsMQTT: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

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

	sTopic = std::string(sLine.substr(0, indexLastSeperator));
	boost::replace_all(sTopic, ";", "/");
	sTopic.insert(0, m_TopicOut + "/");

	sPayload = std::string(sLine.substr(indexLastSeperator + 1));
	if (!sPayload.empty() &&
		sPayload[sPayload.length() - 1] == '\n')
	{
		sPayload.resize(sPayload.length() - 1);
	}
}
