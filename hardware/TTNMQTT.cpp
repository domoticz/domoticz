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

	m_DomLat = 1;
	m_DomLon = 1;

	m_usIPPort = usIPPort;
	m_TopicIn = Username + "/devices/+/up";

#ifdef DEBUG_TTN_R
	std::string sResult = ReadFile("ttn_mqtt_stringfields.json");
	mosquitto_message mqtt_msg;
	mqtt_msg.topic = "ttnmqtt/devices/tektelic_khs/up";
	mqtt_msg.payload = (void*)sResult.c_str();
	mqtt_msg.payloadlen = sResult.size();
	on_message(&mqtt_msg);
#endif

	mosqdz::lib_init();
}

CTTNMQTT::~CTTNMQTT()
{
	mosqdz::lib_cleanup();
}

bool CTTNMQTT::StartHardware()
{
	RequestStart();

	StartHeartbeatThread();

	int nValue;
	std::string sValue;
	std::string Latitude;
	std::string Longitude;

	// Let's get the 'home' Location of this Domoticz instance from the Preferences
	if (m_sql.GetPreferencesVar("Location", nValue, sValue))
	{
		std::vector<std::string> strarray;
		StringSplit(sValue, ";", strarray);

		if (strarray.size() == 2)
		{
			Latitude = strarray[0];
			Longitude = strarray[1];

			if (!((Latitude == "1") && (Longitude == "1")))
			{
				m_DomLat = std::stod(Latitude);
				m_DomLon = std::stod(Longitude);
			}
			else
			{
				Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			}
		}
		else
		{
			Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
		}
	}

	//force connect the next first time
	m_IsConnected = false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
	Log(LOG_STATUS, "Subscribed");
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
			Log(LOG_STATUS, "Re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else {
			Log(LOG_STATUS, "Connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
		}
		subscribe(nullptr, m_TopicIn.c_str());
	}
	else {
		Log(LOG_ERROR, "Connection failed!, restarting (rc=%d)", rc);
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
				Log(LOG_ERROR, "Disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else
			{
				Log(LOG_ERROR, "Disconnected, restarting (rc=%d)", rc);
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
	Log(LOG_STATUS, "Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 60;

	if (!m_CAFilename.empty()) {
		rc = tls_set(m_CAFilename.c_str());
		rc = tls_insecure_set(true);

		if (rc != MOSQ_ERR_SUCCESS)
		{
			Log(LOG_ERROR, "Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		}
		Log(LOG_STATUS, "Enabled TLS mode");
	}
	rc = username_pw_set((!m_UserName.empty()) ? m_UserName.c_str() : nullptr, (!m_Password.empty()) ? m_Password.c_str() : nullptr);

	rc = connect(m_szIPAddress.c_str(), m_usIPPort, keepalive);
	if (rc != MOSQ_ERR_SUCCESS)
	{
		Log(LOG_ERROR, "Failed to start, return code: %d (Check IP/Port)", rc);
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
				m_LastHeartbeat = mytime(nullptr);
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

	Log(LOG_STATUS, "Worker stopped...");
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
			Log(LOG_STATUS, "Not Connected, failed to send message: %s", Message.c_str());
			return;
		}
		publish(nullptr, Topic.c_str(), Message.size(), Message.c_str());
	}
	catch (...)
	{
		Log(LOG_ERROR, "Failed to send message: %s", Message.c_str());
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

Json::Value CTTNMQTT::GetSensorWithChannel(const Json::Value &root, const uint8_t sChannel)
{
	Json::Value ret;
	for (const auto &r : root)
	{
		uint8_t channel = (uint8_t)r["channel"].asInt();

		if ((channel == sChannel) && !r["used"])
			return r;
	}
	return ret;
}

void CTTNMQTT::FlagSensorWithChannelUsed(Json::Value &root, const std::string &stype, const uint8_t sChannel)
{
	for (auto &r : root)
	{
		uint8_t channel = (uint8_t)r["channel"].asInt();
		std::string type = r["type"].asString();

		if ((type == stype) && (channel == sChannel))
		{
			r["used"] = true;
			return;
		}
	}
}

void CTTNMQTT::UpdateUserVariable(const std::string &varName, const std::string &varValue)
{
	std::string szLastUpdate = TimeToString(nullptr, TF_DateTime);

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
	
	//Debug(DEBUG_NORM, "Looking for Device and Sensor (%d): .%s. , .%s.", m_HwdID, DeviceName.c_str(), MacAddress.c_str());

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
			Log(LOG_ERROR, "Problem adding new Device with MacAddress %s !!", MacAddress.c_str());
		}
	}

	if (!result.empty())
	{
		DeviceID = atoi(result[0][0].c_str());
		//Add last received in database ?
	}

	return DeviceID;
}

bool CTTNMQTT::ConvertField2Payload(const std::string &sType, const std::string &sValue, const uint8_t channel, const uint8_t index, Json::Value &payload)
{
	bool ret = false;

	if (sType == "temp" || sType == "temperature") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "temp";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "hum" || sType == "humidity") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "humidity";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "baro") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "baro";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "gps") {
		std::vector<std::string> strarray;
		StringSplit(sValue, ";", strarray);

		if (strarray.size() == 3)
		{
			payload[index]["channel"] = channel;
			payload[index]["type"] = "gps";
			payload[index]["lat"] = std::stod(strarray[0]);
			payload[index]["lon"] = std::stod(strarray[1]);
			payload[index]["alt"] = std::stof(strarray[2]);
			ret = true;
		}
	}
	else if (sType == "digital_input") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "digital_input";
		payload[index]["value"] = std::stoi(sValue);
		ret = true;
	}
	else if (sType == "digital_output") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "digital_output";
		payload[index]["value"] = std::stoi(sValue);
		ret = true;
	}
	else if (sType == "analog_input") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "analog_input";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "analog_output") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "analog_output";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "presense") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "presence";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}
	else if (sType == "luminosity") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "luminosity";
		payload[index]["value"] = std::stof(sValue);
		ret = true;
	}

	// The following IS NOT conforming to the CayenneLPP specification
	// but a 'hack'; When using the TTN External Custom Decoder a payload_fields array is added
	// and within these fields a 'batterylevel' can be created (integer range between 0 and 100)
	if (sType == "batterylevel") {
		payload[index]["channel"] = channel;
		payload[index]["type"] = "batterylevel";
		payload[index]["value"] = std::stoi(sValue);
		ret = true;
	}

	return ret;
}

bool CTTNMQTT::ConvertFields2Payload(const Json::Value &fields, Json::Value &payload)
{
	bool ret = false;
	uint8_t index = 0;

	if (!fields.empty())
	{
		Debug(DEBUG_NORM, "Processing fields payload for %d fields!", fields.size());
		for (const auto &id : fields.getMemberNames())
		{
			if (!(fields[id].isNull()) && ConvertField2Payload(id, fields[id].asString(), index + 1, index, payload))
			{
				Debug(DEBUG_NORM, "Converted field %s !", id.c_str());
				index++;
				ret = true;
			}
		}
	}

	return ret;
}

int CTTNMQTT::CalcDomoticsRssiFromLora(const int gwrssi, const float gwsnr)
{
	int iCalc = 12;

	if (gwsnr > 0)
	{
		// Above noise floor, so that fine
		iCalc = gwrssi;
	}
	else
	{
		// Below noisefloor, little more difficult
		iCalc = gwrssi + (int)std::rint(gwsnr);
	}

	// Range somewhere between -150 and +20
	// rssi between -150 and 0 dBm
	// snr between -20 and +10 dB
	// Domoticz expects something between 0 and 11 of what?
	// But 0 feels weird as how could we measure 'no signal' 
	if (iCalc >= -30 || rint(gwsnr) > 7)
		iCalc = 9;
	else if (iCalc > -40)
		iCalc = 8;
	else if (iCalc > -65)
		iCalc = 7;
	else if (iCalc > -70)
		iCalc = 6;
	else if (iCalc > -80)
		iCalc = 5;
	else if (iCalc > -90)
		iCalc = 4;
	else if (iCalc > -100)
		iCalc = 3;
	else if (iCalc > -110)
		iCalc = 2;
	else
		iCalc = 1;

	return iCalc;
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

#ifdef DEBUG_TTN_R
	_log.Log(LOG_NORM, "TTN_MQTT: Topic: %s", topic.c_str());
	_log.Log(LOG_NORM, "TTN_MQTT: qmsg: %s", qMessage.c_str());
#endif

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
			Log(LOG_ERROR, "Invalid data received from %s ! Unable to parse JSON!", MQTTDeviceName.c_str());
			return;
		}
		if (root["payload_raw"].empty())
		{
			Log(LOG_ERROR, "Invalid data received from %s ! No payload_raw found in JSON data!", MQTTDeviceName.c_str());
			return;
		}

		//Get data from message
		std::string DeviceName = root["dev_id"].asString();
		std::string AppId = root["app_id"].asString();
		std::string DeviceSerial = root["hardware_serial"].asString();
		uint8_t MessagePort = root["port"].asInt();
		std::string lpp = base64_decode(root["payload_raw"].asString());

		//Check if the payload_raw contains valid CayenneLPP structured data
		//TO-DO: The current CayenneLPP Decoder assumes Dynamic Sensor Payload structure and not other possible Sensor payload structures like Packed
		//       The LoRa FramePort should be checked to find out which type of patload structure is used. Port 1 is Dynamic, Port 2 is Packed, etc.
		//       But as for LoRa mostly port 2 is used as the default and Dynamic the most implemented CatenneLPP payload structure, we stick with these assumptions
		Json::Value payload;
		bool Decoded = false;

		switch (MessagePort) {
			case 2:
				if (false) //if (CayenneLPPDec::ParseLPP_Packed((const uint8_t*)lpp.c_str(), lpp.size(), payload))
				{
					Decoded = true;
				}
				break;
			case 10:
				if (false) //if (Tektelic::ParseKHS((const uint8_t*)lpp.c_str(), lpp.size(), payload))
				{
					Decoded = true;
				}
				break;
			case 1:
			default:
				if(CayenneLPPDec::ParseLPP((const uint8_t*)lpp.c_str(), lpp.size(), payload))
				{
					Decoded = true;
				}
		}

		if (!Decoded)
		{
			if (!root["payload_fields"].empty())
			{
				// Maybe we received a pre-decoded message? TTN has the option for custom decoders where the decoding is done by TTN already
				if (!ConvertFields2Payload(root["payload_fields"], payload))
				{
					Log(LOG_ERROR, "Invalid data received! Unable to decode the raw payload and the fields payload does not contain any (valid) data!");
					return;
				}
				Log(LOG_STATUS, "Converted payload_fields to regular payload for processing!");
			}
			else
			{
				Log(LOG_ERROR, "Invalid data received! The payload_raw does not contain a payload that could be decoded (Port %d)!", MessagePort);
				return;
			}
		}

		if ((payload.empty()) || (!payload.isArray()))
		{
			Log(LOG_ERROR, "Invalid data received! The payload doesn't contain (valid) data!");
			return;
		}

		int DeviceID = GetAddDeviceAndSensor(m_HwdID, DeviceName, DeviceSerial);
		if (DeviceID == 0) // Unable to find the Device and/or Add the new Device
		{
			return;
		}

		//See what info we can use from the provided metadata and gateway(s) data
		const std::string format = "%Y-%m-%dT%H:%M:%S";
		time_t msgtime = 0;
		double ssrlat = 0;
		double ssrlon = 0;
		float ssralt = 0;
		double devlat = 0;
		double devlon = 0;
		float devalt = 0;
		double gwlat = 0;
		double gwlon = 0;
		float gwalt = 0;
		int gwrssi = -999;
		float gwsnr = -999;

		// Let's look at the metadata that TTN also sends when receiving a LoRa message
		if (!root["metadata"].empty())
		{
			Json::Value MetaData = root["metadata"];
			if (!MetaData["time"].empty())
			{
				// Retrieve the moment the TTN backend receives the (first part of) this packet
				// So we have a more accurate time when the measurement happened, compared to
				// the moment we see the message here in Domoticz
				// TTN time is in ISO8601 format and UTC
				std::tm t = {};
				int y,M,d,h,m;
				float s;
				const char* UTCttntime;

				UTCttntime = MetaData["time"].asString().c_str();
				sscanf(UTCttntime, "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);
				constructTime(msgtime, t, y, M, d, h, m, (int)floor(s));
			}
			if (!(MetaData["latitude"].empty() || MetaData["longitude"].empty()))
			{
				// For this device, Location coordinates are set in the metadata.
				// Makes sense for non moving sensors without own GPS
				devlat = MetaData["latitude"].asDouble();
				devlon = MetaData["longitude"].asDouble();
				if (!MetaData["altitude"].empty())
				{
					devalt = MetaData["altitude"].asFloat();
				}
			}
			// Let's see if there is metadata from 1 or more gateways that have received this message
			if (!MetaData["gateways"].empty())
			{
				Json::Value Gateways = MetaData["gateways"];
				// Loop over all gateways and try to find the one with the best signal
				// and see if the GW has a Geo Location that we could use
				uint8_t iGW = 0;
				do
				{
					if (!Gateways[iGW].empty())
					{
						Json::Value Gateway = Gateways[iGW];

						int lrssi = Gateway["rssi"].asInt();
						float lsnr = Gateway["snr"].asFloat();
						bool bBetter = false;
						bool bGwGeo = (!(Gateway["latitude"].empty() || Gateway["longitude"].empty()));
						bool bPrevGwGeo = (!(gwlat == 0 || gwlon == 0));

						// Is this gateway closer to the sensor than the previous one (or is this the first/only one)
						if (lsnr > 0 && floor(lsnr) >= floor(gwsnr))
						{
							if (floor(lsnr) == floor(gwsnr))
							{
								if(!bGwGeo && bPrevGwGeo)
								{
									if (floor((lrssi/10)) > floor((gwrssi/10)))
									{
										bBetter = true;
									}
								}
								else if (bGwGeo && !bPrevGwGeo)
								{
									bBetter = true;
								}
								else
								{
									// Current and previous both either do or do not have Geo data
									if (lrssi > gwrssi)
									{
										bBetter = true;
									}
								}
							}
							else
							{
								// Postitive SNR is better by a full point at least
								bBetter = true;
							}
						}
						else if (lsnr <= 0 && lrssi > gwrssi)
						{
							if(!bGwGeo && bPrevGwGeo)
							{
								// The previous found closest GW has Geo info and this one doesn't
								// If the signals are very simular, we prefer the one with Geo
								if (floor((lrssi/10)) > floor((gwrssi/10)))
								{
									bBetter = true;
								}
							}
							else
							{
								bBetter = true;
							}
						}

						if (bBetter)
						{
							// Ok, this gateway seems to be closer
							// Let's see if it has Geo Location data
							if (bGwGeo)
							{
								gwlat = Gateway["latitude"].asDouble();
								gwlon = Gateway["longitude"].asDouble();
								if (!Gateway["altitude"].empty())
								{
									gwalt = Gateway["altitude"].asFloat();
								}
							}
							else if (bPrevGwGeo)
							{
 								// No Geo data for this gateway, but we have a previous gateway further away. See clear the gwloc data
								gwlat = 0;
								gwlon = 0;
								gwalt = 0;
							}
							gwsnr = lsnr;
							gwrssi = lrssi;
						}
					}
					iGW++;
				}
				while (!Gateways[iGW].empty());
				rssi = CalcDomoticsRssiFromLora(gwrssi, gwsnr);
			}
		}

		// Walk over the payload to find all used channels. Each channel represents a single sensor.
		uint8_t chanSensors [65] = {};	// CayenneLPP Data Channel ranges from 0 to 64
		for (const auto &p : payload)
		{
			std::string type = p["type"].asString();
			uint8_t channel = (uint8_t)p["channel"].asInt();

			// The following IS NOT part of the CayenneLPP specification
			// But a 'hack'; when using the payload_fields (using the TTN external payload decoder)
			// a 'batterylevel' field can be created (integer range between 0 and 100)
			// Also, we do skip this reading for further processing
			if (type == "batterylevel") {
				if (p["value"].isNumeric())
				{
					BatteryLevel = (int)p["value"].asInt();
				}
			}
			else
			{
				// Add 1 reading to this channel
				chanSensors[channel]++;
			}
			Debug(DEBUG_RECEIVED, "Increased count for channel %d (%s)!", channel, type.c_str());
		}

		// Now walk over each channel/sensor to find the different measurement types and values
		uint8_t channel = 0;
		uint8_t iGps = 0;
		uint8_t iGpsChannel = 0;
		do {
			if(chanSensors[channel] > 0)
			{
				Debug(DEBUG_RECEIVED, "Processing %d Sensorvalues for channel %d!", chanSensors[channel],channel);
				bool bTemp = false, bHumidity = false, bBaro = false, bDin = false, bDout = false, bAin = false, bAout = false, bPresense = false, bLuminosity = false;
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
						Debug(DEBUG_RECEIVED, "Processing Sensor of type %s for channel %d!", type.c_str(),channel);

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
							ssrlat = vSensor["lat"].asDouble();
							ssrlon = vSensor["lon"].asDouble();
							ssralt = vSensor["alt"].asFloat();
							//SendGpsLocation(DeviceID, channel, BatteryLevel, rssi, DeviceName, ssrlat, ssrlon, ssralt);

							std::stringstream sstr;
							sstr << ssrlat << ";" << ssrlon << ";" << ssralt;
							SendCustomSensor(DeviceID, channel, BatteryLevel, ssralt, DeviceName + " Geo", "meters", rssi);
							UpdateUserVariable(DeviceName + " Geo", sstr.str());

							iGps++;
							iGpsChannel = channel;

						}
						else if (type == "digital_input") {
							SendGeneralSwitch(DeviceID, channel, BatteryLevel, vSensor["value"].asInt(), 0, DeviceName, m_Name, rssi);
							bDin = true;
						}
						else if (type == "digital_output") {
							SendGeneralSwitch(DeviceID, channel, BatteryLevel, vSensor["value"].asInt(), 0, DeviceName, m_Name, rssi);
							bDout = true;
						}
						else if (type == "analog_input") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type, rssi);
							bAin = true;
						}
						else if (type == "analog_output") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type, rssi);
							bAout = true;
						}
						else if (type == "presense") {
							SendCustomSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName, type, rssi);
							bPresense = true;
						}
						else if (type == "luminosity") {
							SendLuxSensor(DeviceID, channel, BatteryLevel, vSensor["value"].asFloat(), DeviceName);
							bLuminosity = true;
						}
						else if (type == "accel") {
							Log(LOG_STATUS, "Sensortype %s not implemented!", type.c_str());
						}
						else if (type == "gyro") {
							Log(LOG_STATUS, "Sensortype %s not implemented!", type.c_str());
						}
						else if (type == "unixtime") {
							Log(LOG_STATUS, "Sensortype %s not implemented!", type.c_str());
						}
						else {
							Log(LOG_STATUS, "Unhandled Sensortype %s!", type.c_str());
						}

						FlagSensorWithChannelUsed(payload, type, channel);
					}
					else
					{
						Log(LOG_ERROR, "Could not process all Sensorvalues for channel %d! Now at %d, but expected %d values!", channel, current, chanSensors[channel]);
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
				// Create a 'better' DeviceID for some sensors
				int iAltDevId = (DeviceID << 8) | channel;
				if (bTemp && bHumidity && bBaro)
				{
					SendTempHumBaroSensorFloat(iAltDevId, BatteryLevel, temp, (int)std::rint(hum), baro, (uint8_t)nforecast, DeviceName, rssi);
				}
				else if(bTemp && bHumidity)
				{
					SendTempHumSensor(iAltDevId, BatteryLevel, temp, (int)std::rint(hum), DeviceName, rssi);
				}
				else if(bTemp && bBaro)
				{
					SendTempBaroSensor(DeviceID, BatteryLevel, temp, baro, DeviceName);
				}
				else
				{
					if (bTemp)
					{
						SendTempSensor(iAltDevId, BatteryLevel, temp, DeviceName, rssi);
					}
					if (bHumidity)
					{
						SendHumiditySensor(iAltDevId, BatteryLevel, (int)std::rint(hum), DeviceName, rssi);
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

		//Debug(DEBUG_NORM, "Completed processing of readings!");

		// Now we have processed all readings for all channels
		// If we have not seen any GPS data on any channel in this payload
		// and the metadata contains Location coordinates, let's process these 
		if(iGps == 0 && !(devlat == 0 || devlon == 0))
		{
			std::stringstream sstr;
			sstr << devlat << ";" << devlon << ";" << devalt;

			SendCustomSensor(DeviceID, channel, BatteryLevel, devalt, DeviceName + " Geo", "meters", rssi);
			UpdateUserVariable(DeviceName + " Geo", sstr.str());

			// Set the Meta location as sensor location for further processing
			ssrlat = devlat;
			ssrlon = devlon;
			ssralt = devalt;

			iGpsChannel = channel;
		}

		// If we have a Geo Location of the sensor (either own or meta), calculate distance from 'home'
		if(iGps == 1 || !(devlat == 0 || devlon == 0))
		{
			uint64_t nSsrDistance = static_cast<int>(std::rint(1000 * distanceEarth(m_DomLat, m_DomLon, ssrlat, ssrlon)));
			SendCustomSensor(DeviceID, (iGpsChannel + 64), BatteryLevel, (float)nSsrDistance, DeviceName + " Home Distance", "meters", rssi);
			Debug(DEBUG_NORM, "Distance between Sensordevice and Domoticz Home is %f meters!", (double)nSsrDistance);
		}

		// Did we find any Geo Location data from the Gateway with the best reception
		if (!(gwlat == 0 && gwlon == 0))
		{
			Debug(DEBUG_NORM, "Found Geo Location data for the Gateway with the best reception at lat: %f, lon: %f, alt: %f", gwlat, gwlon, gwalt);
			// We have Geo Location data of the sensor AND of the gateway, so we can calculate the distance
			if (iGps > 1)
			{
				Log(LOG_STATUS, "More than 1 GPS measurements found! Unable to determine which one to pick for distance calculations!"); 
			}
			else if (!(ssrlat == 0 && ssrlon == 0))
			{
				uint64_t nGwDistance = static_cast<int>(std::rint(1000 * distanceEarth(gwlat, gwlon, ssrlat, ssrlon)));
				SendCustomSensor(DeviceID, (channel + 128), BatteryLevel, (float)nGwDistance, DeviceName + " Gateway Distance", "meters", rssi);
				Debug(DEBUG_NORM, "Distance between Sensordevice and gateway is %f meters!", (double)nGwDistance);
			}
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error parsing message!!!");
	}
}
