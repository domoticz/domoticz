#include "stdafx.h"
#include "TTNMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../json/json.h"
#include "../webserver/Base64.h"
#include "cayenne_lpp/CayenneLPP_Dec.h"
#include <sstream>

#define RETRY_DELAY 30

#define CLIENTID	"Domoticz"
#define QOS         1

#ifdef _DEBUG
//#define DEBUG_TTN_R
//#define DEBUG_TTN_W
#endif

#ifdef DEBUG_TTN_W
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_TTN_R
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif


CTTNMQTT::CTTNMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename) :
	mosquittopp(CLIENTID),
	m_szIPAddress(IPAddress),
	m_UserName(Username),
	m_Password(Password),
	m_CAFilename(CAfilename)
{
	m_HwdID = ID;
	m_IsConnected = false;
	m_bDoReconnect = false;

	m_usIPPort = usIPPort;
	m_TopicIn = Username + "/devices/+/up";

#ifdef DEBUG_TTN_R
	std::string sResult = ReadFile("E:\\ttn_mqtt.json");
	mosquitto_message mqtt_msg;
	mqtt_msg.topic = "domo_rob/devices/lopy4/up";
	mqtt_msg.payload = (void*)sResult.c_str();
	mqtt_msg.payloadlen = sResult.size();
	on_message(&mqtt_msg);
#endif

	mosqpp::lib_init();
}

CTTNMQTT::~CTTNMQTT(void)
{
	mosqpp::lib_cleanup();
}

bool CTTNMQTT::StartHardware()
{
	RequestStart();

	StartHeartbeatThread();

	//force connect the next first time
	m_IsConnected = false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CTTNMQTT::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

void CTTNMQTT::StopMQTT()
{
	disconnect();
	m_bIsStarted = false;
}

bool CTTNMQTT::StopHardware()
{
	StopHeartbeatThread();
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_IsConnected = false;
	return true;
}

void CTTNMQTT::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	_log.Log(LOG_STATUS, "TTN_MQTT: Subscribed");
	m_IsConnected = true;
}

void CTTNMQTT::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	if (rc == 0) {
		if (m_IsConnected) {
			_log.Log(LOG_STATUS, "TTN_MQTT: re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else {
			_log.Log(LOG_STATUS, "TTN_MQTT: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
		}
		subscribe(NULL, m_TopicIn.c_str());
	}
	else {
		_log.Log(LOG_ERROR, "TTN_MQTT: Connection failed!, restarting (rc=%d)", rc);
		m_bDoReconnect = true;
	}
}

void CTTNMQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!IsStopRequested(0))
		{
			if (rc == 5)
			{
				_log.Log(LOG_ERROR, "TTN_MQTT: disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else
			{
				_log.Log(LOG_ERROR, "TTN_MQTT: disconnected, restarting (rc=%d)", rc);
			}
			m_bDoReconnect = true;
		}
	}
}


bool CTTNMQTT::ConnectInt()
{
	StopMQTT();
	return ConnectIntEx();
}

bool CTTNMQTT::ConnectIntEx()
{
	m_bDoReconnect = false;
	_log.Log(LOG_STATUS, "TTN_MQTT: Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 60;

	if (!m_CAFilename.empty()) {
		rc = tls_set(m_CAFilename.c_str());

		if (rc != MOSQ_ERR_SUCCESS)
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		}
		else {
			_log.Log(LOG_STATUS, "TTN_MQTT: enabled TLS mode");
		}
	}
	rc = username_pw_set((!m_UserName.empty()) ? m_UserName.c_str() : NULL, (!m_Password.empty()) ? m_Password.c_str() : NULL);

	rc = connect(m_szIPAddress.c_str(), m_usIPPort, keepalive);
	if (rc != MOSQ_ERR_SUCCESS)
	{
		_log.Log(LOG_ERROR, "TTN_MQTT: Failed to start, return code: %d (Check IP/Port)", rc);
		m_bDoReconnect = true;
		return false;
	}
	return true;
}

void CTTNMQTT::Do_Work()
{
	bool bFirstTime = true;
	int msec_counter = 0;
	int sec_counter = 0;

	while (!IsStopRequested(100))
	{
		if (!bFirstTime)
		{
			int rc = loop();
			if (rc) {
				if (rc != MOSQ_ERR_NO_CONN)
				{
					if (!IsStopRequested(0))
					{
						if (!m_bDoReconnect)
						{
							reconnect();
						}
					}
				}
			}
		}

		msec_counter++;
		if (msec_counter == 10)
		{
			msec_counter = 0;

			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}

			if (bFirstTime)
			{
				bFirstTime = false;
				ConnectInt();
			}
			else
			{
				if (sec_counter % 30 == 0)
				{
					if (m_bDoReconnect)
						ConnectIntEx();
				}
				if (isConnected() && sec_counter % 10 == 0)
				{
					SendHeartbeat();
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "TTN_MQTT: Worker stopped...");
}

void CTTNMQTT::SendHeartbeat()
{
	// not necessary for normal MQTT servers
}

void CTTNMQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	try {
		if (!m_IsConnected)
		{
			_log.Log(LOG_STATUS, "TTN_MQTT: Not Connected, failed to send message: %s", Message.c_str());
			return;
		}
		publish(NULL, Topic.c_str(), Message.size(), Message.c_str());
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "TTN_MQTT: Failed to send message: %s", Message.c_str());
	}
}

void CTTNMQTT::WriteInt(const std::string &sendStr)
{
	if (sendStr.size() < 2)
		return;
	//string the return and the end
	//std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size());
	//SendMessage(m_TopicOut, sMessage);
}

Json::Value CTTNMQTT::GetSensorWithChannel(const Json::Value &root, const std::string &stype, const int sChannel)
{
	Json::Value ret;
	for (auto itt = root.begin(); itt != root.end(); ++itt)
	{
		uint8_t channel = (uint8_t)(*itt)["channel"].asInt();
		std::string type = (*itt)["type"].asString();

		if ((type == stype) && (channel == sChannel))
			return (*itt);
	}
	return ret;
}

void CTTNMQTT::FlagSensorWithChannelUsed(Json::Value &root, const std::string &stype, const int sChannel)
{
	for (auto itt = root.begin(); itt != root.end(); ++itt)
	{
		uint8_t channel = (uint8_t)(*itt)["channel"].asInt();
		std::string type = (*itt)["type"].asString();

		if ((type == stype) && (channel == sChannel))
		{
			(*itt)["used"] = true;
			return;
		}
	}
}

void CTTNMQTT::UpdateUserVariable(const std::string &varName, const std::string &varValue)
{
	std::string szLastUpdate = TimeToString(NULL, TF_DateTime);

	int ID;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE (Name=='%q')", varName.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", varName.c_str(), USERVARTYPE_STRING, varValue.c_str());
		result = m_sql.safe_query("SELECT ID FROM UserVariables WHERE (Name=='%q')", varName.c_str());
		if (result.empty())
			return;
		ID = atoi(result[0][0].c_str());
	}
	else
	{
		ID = atoi(result[0][0].c_str());
		m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%q' WHERE (ID==%d)", varValue.c_str(), szLastUpdate.c_str(), ID);
	}

	m_mainworker.m_eventsystem.SetEventTrigger(ID, m_mainworker.m_eventsystem.REASON_USERVARIABLE, 0);
	m_mainworker.m_eventsystem.UpdateUserVariable(ID, varValue, szLastUpdate);
}


void CTTNMQTT::on_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;

	if (topic.find("/up/") != std::string::npos)
		return; //not interested in sub-topics

	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

#ifdef DEBUG_TTN_W
	SaveString2Disk(qMessage, "E:\\ttn_mqtt.json");
#endif

	_log.Log(LOG_NORM, "TTN_MQTT: Topic: %s", topic.c_str());

	if (qMessage.empty())
		return;

	try {
		Json::Value root;
		Json::Reader jReader;
		bool ret = jReader.parse(qMessage, root);
		if ((!ret) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received!");
			return;
		}
		if (root["payload_raw"].empty())
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received!");
			return;
		}

		///Get device name
		std::vector<std::string> strarray;
		StringSplit(topic, "/", strarray);

		std::string DeviceName = strarray[2];

		int DeviceID = 0;

		//Get our internal device_id, if not found, add it
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress==\"%q\")", m_HwdID, DeviceName.c_str());
		if (result.empty())
		{
			//New one, let's add it to the system
			m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (%d, '%q', '%q')", m_HwdID, DeviceName.c_str(), DeviceName.c_str());
			result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress==\"%q\")", m_HwdID, DeviceName.c_str());
			if (result.empty())
			{
				_log.Log(LOG_ERROR, "TTN_MQTT Hue: Problem adding new Device!!");
				return;
			}
		}
		DeviceID = atoi(result[0][0].c_str());
		//Add last received in database ?

		int rssi = 12;	//Get the best RSSI from the Gateways received
		//Add rssi in database, and/or gateways received

		std::string lpp = base64_decode(root["payload_raw"].asString());
		Json::Value payload;

		if (!CayenneLPPDec::ParseLPP((const uint8_t*)lpp.c_str(), lpp.size(), payload))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received!");
			return;
		}

		if ((payload.empty()) || (!payload.isArray()))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received!");
			return;
		}
		for (auto itt = payload.begin(); itt != payload.end(); ++itt)
		{
			if (!(*itt)["used"].empty())
				continue;
			uint8_t channel = (uint8_t)(*itt)["channel"].asInt();
			std::string type = (*itt)["type"].asString();

			int BatteryLevel = 255;

			if (
				(type == "temp")
				|| (type == "humidity")
				|| (type == "baro")
				)
			{
				Json::Value vTemp = GetSensorWithChannel(payload, "temp", channel);
				Json::Value vHum = GetSensorWithChannel(payload, "humidity", channel);
				Json::Value vBaro = GetSensorWithChannel(payload, "baro", channel);

				bool bHaveTemp = vTemp.isObject();
				bool bHaveHumidity = vHum.isObject();
				bool bHaveBaro = vBaro.isObject();
				int nforecast = wsbaroforcast_some_clouds;
				float temp, hum, baro;
				if (bHaveTemp)
				{
					temp = vTemp["value"].asFloat();
				}
				if (bHaveHumidity)
				{
					hum = vHum["value"].asFloat();
				}
				if (bHaveBaro)
				{
					baro = vBaro["value"].asFloat();
					if (bHaveTemp)
						nforecast = m_forecast_calculators[DeviceName].CalculateBaroForecast(temp, baro);
					else
						nforecast = m_forecast_calculators[DeviceName].CalculateBaroForecast(baro);
				}
				if (bHaveTemp && bHaveHumidity && bHaveBaro)
				{
					SendTempHumBaroSensorFloat(DeviceID, BatteryLevel, temp, (int)rint(hum), baro, nforecast, DeviceName, rssi);
				}
				else if (bHaveTemp && bHaveHumidity)
				{
					SendTempHumSensor(DeviceID, BatteryLevel, temp, (int)rint(hum), DeviceName, rssi);
				}
				else if (bHaveTemp && bHaveBaro)
				{
					SendTempBaroSensor(DeviceID, BatteryLevel, temp, baro, DeviceName);
				}
				else
				{
					if (bHaveTemp)
					{
						SendTempSensor(DeviceID, BatteryLevel, temp, DeviceName, rssi);
					}
					if (bHaveHumidity)
					{
						SendHumiditySensor(DeviceID, BatteryLevel, (int)rint(hum), DeviceName, rssi);
					}
					if (bHaveBaro)
					{
						SendBaroSensor(DeviceID, 1, BatteryLevel, baro, nforecast, DeviceName);
					}
				}
				if (bHaveTemp)
					FlagSensorWithChannelUsed(payload, "temp", channel);
				if (bHaveHumidity)
					FlagSensorWithChannelUsed(payload, "humidity", channel);
				if (bHaveBaro)
					FlagSensorWithChannelUsed(payload, "baro", channel);
			}
			else if (type == "gps")
			{
				float height = (*itt)["alt"].asFloat();
				SendPercentageSensor(DeviceID, 1, BatteryLevel, height, DeviceName + " Altitude");

				std::stringstream sstr;
				sstr << (*itt)["lat"].asFloat() << "," << (*itt)["lon"].asFloat() << "," << (*itt)["alt"].asFloat();

				UpdateUserVariable(DeviceName, sstr.str());
			}
			else if ((type == "digital_input") || (type == "digital_output"))
			{
				SendGeneralSwitch(DeviceID, 1, BatteryLevel, (*itt)["value"].asInt(), 0, DeviceName, rssi);
			}
			else if ((type == "analog_input") || (type == "analog_output") || (type == "presence"))
			{
				SendVoltageSensor(DeviceID, 1, BatteryLevel, (*itt)["value"].asFloat(), DeviceName);
			}
			else if (type == "luminosity")
			{
				SendLuxSensor(DeviceID, 1, BatteryLevel, (*itt)["value"].asFloat(), DeviceName);
			}
			else
			{
				_log.Log(LOG_STATUS, "TTN_MQTT: Unhandled type: %s (Please report!)", type.c_str());
			}

			(*itt)["used"] = true;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "TTN_MQTT: Error parsing message!!!");
	}
}
