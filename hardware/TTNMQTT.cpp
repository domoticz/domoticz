#include "stdafx.h"
#include "TTNMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../webserver/Base64.h"
#include "cayenne_lpp/CayenneLPP_Dec.h"
#include <sstream>

#define RETRY_DELAY 30

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
	mosqdz::mosquittodz((std::string("Domoticz-TTN") +  std::string(GenerateUUID())).c_str()),
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
	std::string sResult = ReadFile("ttn_mqtt.json");
	mosquitto_message mqtt_msg;
	mqtt_msg.topic = "domo_rob/devices/lopy4/up";
	mqtt_msg.payload = (void*)sResult.c_str();
	mqtt_msg.payloadlen = sResult.size();
	on_message(&mqtt_msg);
#endif

	mosqdz::lib_init();
}

CTTNMQTT::~CTTNMQTT(void)
{
	mosqdz::lib_cleanup();
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
	clear_callbacks();

	if (isConnected())
		disconnect();

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

Json::Value CTTNMQTT::GetSensorWithChannel(const Json::Value &root, const int sChannel)
{
	Json::Value ret;
	for (auto itt = root.begin(); itt != root.end(); ++itt)
	{
		uint8_t channel = (uint8_t)(*itt)["channel"].asInt();

		if ((channel == sChannel) && !(*itt)["used"])
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

int CTTNMQTT::GetAddDeviceAndSensor(const int m_HwdID, const std::string &DeviceName, const std::string &MacAddress)
{
	int DeviceID = 0;
	
	//_log.Log(LOG_NORM, "TTN_MQTT: Looking for Device and Sensor (%i): .%s. , .%s.", m_HwdID, DeviceName.c_str(), MacAddress.c_str());

	//Get our internal device_id, if not found, add it
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, MacAddress.c_str());
	if (result.empty())
	{
		//New one, let's add it to the system
		m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (%d, '%q', '%q')", m_HwdID, DeviceName.c_str(), MacAddress.c_str());
		result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (MacAddress=='%q')", m_HwdID, MacAddress.c_str());
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Problem adding new Device with MacAddress %s !!", MacAddress.c_str());
		}
	}

	if (!result.empty())
	{
		DeviceID = atoi(result[0][0].c_str());
		//Add last received in database ?
	}

	return DeviceID;
}

void CTTNMQTT::on_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;

	if (topic.find("/up/") != std::string::npos)
		return; //not interested in sub-topics

	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

#ifdef DEBUG_TTN_W
	SaveString2Disk(qMessage, "ttn_mqtt.json");
#endif

	//_log.Log(LOG_NORM, "TTN_MQTT: Topic: %s", topic.c_str());

	if (qMessage.empty())
		return;

	try {
		int rssi = 12;	//Set a default, means unknown
		int BatteryLevel = 255;	// BatteryLevel is not measured, but needs to be provided

		//Get device name from MQTT topic
		std::vector<std::string> strarray;
		StringSplit(topic, "/", strarray);
		std::string MQTTDeviceName = strarray[2];

		//Check if we received a JSON object with payload_raw
		Json::Value root;
		bool ret = ParseJSon(qMessage, root);
		if ((!ret) || (!root.isObject()))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received from %s ! Unable to parse JSON!", MQTTDeviceName.c_str());
			return;
		}
		if (root["payload_raw"].empty())
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received from %s ! No payload_raw found in JSON data!", MQTTDeviceName.c_str());
			return;
		}

		//Get data from message
		std::string DeviceName = root["dev_id"].asString();
		std::string AppId = root["app_id"].asString();
		std::string DeviceSerial = root["hardware_serial"].asString();
		int MessagePort = root["port"].asInt();
		std::string lpp = base64_decode(root["payload_raw"].asString());

		//Check if the payload_raw contains valid CayenneLPP structured data
		//TO-DO: The current CayenneLPP Decoder assumes Dynamic Sensor Payload structure and not other possible Sensor payload structures like Packed
		//       The LoRa FramePort should be checked to find out which type of patload structure is used. Port 1 is Dynamic, Port 2 is Packed, etc.
		//       But as for LoRa mostly port 2 is used as the default and Dynamic the most implemented CatenneLPP payload structure, we stick with these assumptions
		Json::Value payload;

		if (!CayenneLPPDec::ParseLPP((const uint8_t*)lpp.c_str(), lpp.size(), payload))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received! The payload_raw does not contain a valid CayenneLPP payload!");
			return;
		}

		if ((payload.empty()) || (!payload.isArray()))
		{
			_log.Log(LOG_ERROR, "TTN_MQTT: Invalid data received! The CayenneLPP payload doesn't contain (valid) data!");
			return;
		}

		//See if we can find the rssi
		if (!root["metadata"].empty())
		{
			Json::Value MetaData = root["metadata"];
			if (!MetaData["gateways"].empty())
			{
				Json::Value Gateways = MetaData["gateways"];
				// TO_DO: there could be multiple gateways and we should loop them to find the lowest rssi
				if (!Gateways[0].empty())
				{
					Json::Value Gateway = Gateways[0];
					int rssi = Gateway["rssi"].asInt();
					//_log.Log(LOG_NORM, "TTN_MQTT: Found Gateway 0! RSSI %i", rssi);
				}
			}
		}

		int DeviceID = GetAddDeviceAndSensor(m_HwdID, DeviceName.c_str(), DeviceSerial.c_str());
		if (DeviceID == 0) // Unable to find the Device and/or Add the new Device
		{
			return;
		}

		// Walk over the payload to find all used channels. Each channel represents a single sensor.
		int chanSensors [65] = {};	// CayenneLPP Data Channel ranges from 0 to 64
		for (auto itt = payload.begin(); itt != payload.end(); ++itt)
		{
			uint8_t channel = (uint8_t)(*itt)["channel"].asInt();
			std::string type = (*itt)["type"].asString();

			chanSensors[channel]++;
			//_log.Log(LOG_STATUS, "TTN_MQTT: Increased count for channel %i (%s)!", channel, type.c_str());
		}

		// Now walk over each channel/sensor to find the different measurement types and values
		int channel = 0;
		do {
			if(chanSensors[channel] > 0)
			{
				//_log.Log(LOG_STATUS, "TTN_MQTT: Processing %i Sensorvalues for channel %i!", chanSensors[channel],channel);
				bool bTemp = false, bHumidity = false, bBaro = false, bGps = false, bDin = false, bDout = false, bAin = false, bAout = false, bPresense = false, bLuminosity = false;
				float temp = 0, hum = 0, baro = 0, lat = 0, lon = 0, alt = 0, ain = 0, aout = 0, presence = 0, luminocity = 0;
				int din = 0, dout = 0;
				uint8_t nforecast = wsbaroforecast_some_clouds;

				int current = 1;
				do {
					// Look for Sensorreading for the sensor of this channel that hasn't been processed yet
					Json::Value vSensor = GetSensorWithChannel(payload, channel);
					if ( vSensor.isObject() )
					{
						std::string type = vSensor["type"].asString();
						//_log.Log(LOG_STATUS, "TTN_MQTT: Processing Sensor of type %s for channel %i!", type.c_str(),channel);

						if (type == "temp") {
							temp = vSensor["value"].asFloat();
							bTemp = true;
						}
						else if (type == "humidity") {
							hum = vSensor["value"].asFloat();
							bHumidity = true;
						}
						else if (type == "baro") {
							baro = vSensor["value"].asFloat();
							bBaro = true;
						}
						else if (type == "gps") {
							bGps = true;
						}
						else if (type == "digital_input") {
							SendGeneralSwitch(DeviceID, channel, BatteryLevel, vSensor["value"].asInt(), 0, DeviceName, rssi);
							bDin = true;
						}
						else if (type == "digital_output") {
							SendGeneralSwitch(DeviceID, channel, BatteryLevel, vSensor["value"].asInt(), 0, DeviceName, rssi);
							bDout = true;
						}
						else if (type == "analog_input") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type);
							bAin = true;
						}
						else if (type == "analog_output") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type);
							bAout = true;
						}
						else if (type == "presense") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type);
							bPresense = true;
						}
						else if (type == "luminosity") {
							SendLuxSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName);
							bLuminosity = true;
						}
						else if (type == "accel") {
							_log.Log(LOG_STATUS, "TTN_MQTT: Sensortype %s not implemented!", type.c_str());
						}
						else if (type == "gyro") {
							_log.Log(LOG_STATUS, "TTN_MQTT: Sensortype %s not implemented!", type.c_str());
						}
						else if (type == "unixtime") {
							_log.Log(LOG_STATUS, "TTN_MQTT: Sensortype %s not implemented!", type.c_str());
						}
						else {
							_log.Log(LOG_STATUS, "TTN_MQTT: Unhandled Sensortype %s!", type.c_str());
						}

						FlagSensorWithChannelUsed(payload, type, channel);
					}
					else
					{
						_log.Log(LOG_ERROR, "TTN_MQTT: Could not process all Sensorvalues for channel %i! Now at %i, but expected %i values!", channel, current, chanSensors[channel]);
					}
					current++;
				}
				while (current <= chanSensors[channel]);

				// Let see if we can predict a forecast based on the barometer value
				if(bBaro)
				{
					if (bTemp)
					{
						nforecast = m_forecast_calculators[DeviceName].CalculateBaroForecast(temp, baro);
					}
					else
					{
						nforecast = m_forecast_calculators[DeviceName].CalculateBaroForecast(baro);
					}
				}

				// Now store the sensor values if not stored already
				if (bTemp && bHumidity && bBaro)
				{
					SendTempHumBaroSensorFloat(DeviceID, BatteryLevel, temp, (int)rint(hum), baro, (uint8_t)nforecast, DeviceName, rssi);
				}
				else if(bTemp && bHumidity)
				{
					SendTempHumSensor(DeviceID, BatteryLevel, temp, (int)rint(hum), DeviceName, rssi);
				}
				else if(bTemp && bBaro)
				{
					SendTempBaroSensor(DeviceID, BatteryLevel, temp, baro, DeviceName);
				}
				else
				{
					if (bTemp)
					{
						SendTempSensor(DeviceID, BatteryLevel, temp, DeviceName, rssi);
					}
					if (bHumidity)
					{
						SendHumiditySensor(DeviceID, BatteryLevel, (int)rint(hum), DeviceName, rssi);
					}
					if (bBaro)
					{
						SendBaroSensor(DeviceID, channel, BatteryLevel, baro, (uint8_t)nforecast, DeviceName);
					}
				}
			}
			channel++;
		}
		while (channel < 65);
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "TTN_MQTT: Error parsing message!!!");
	}
}
