#include "stdafx.h"
#include "OctoPrintMQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../webserver/Base64.h"
#include <sstream>

#define RETRY_DELAY 30

#ifdef _DEBUG
//#define DEBUG_OCTO_R
//#define DEBUG_OCTO_W
#endif

#define OCTOPRINT_MQTT_TOPIC "octoPrint"
#define TID_PRINTING 1
#define TID_PATH 1

#ifdef DEBUG_OCTO_W
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
#ifdef DEBUG_OCTO_R
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


COctoPrintMQTT::COctoPrintMQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename) :
	mosqdz::mosquittodz((std::string("Domoticz-OCTO") +  std::string(GenerateUUID())).c_str()),
	m_szIPAddress(IPAddress),
	m_UserName(Username),
	m_Password(Password),
	m_CAFilename(CAfilename)
{
	m_HwdID = ID;
	m_IsConnected = false;
	m_bDoReconnect = false;

	m_usIPPort = usIPPort;
	m_TopicIn = std::string(OCTOPRINT_MQTT_TOPIC) + "/#";

	mosqdz::lib_init();
}

COctoPrintMQTT::~COctoPrintMQTT()
{
	mosqdz::lib_cleanup();
}

bool COctoPrintMQTT::StartHardware()
{
	RequestStart();

	StartHeartbeatThread();

	//force connect the next first time
	m_IsConnected = false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	return (m_thread != nullptr);
}

void COctoPrintMQTT::StopMQTT()
{
	disconnect();
	m_bIsStarted = false;
}

bool COctoPrintMQTT::StopHardware()
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

void COctoPrintMQTT::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	_log.Log(LOG_STATUS, "OCTO_MQTT: Subscribed");
	m_IsConnected = true;
}

void COctoPrintMQTT::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	if (rc == 0) {
		if (m_IsConnected) {
			_log.Log(LOG_STATUS, "OCTO_MQTT: re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else {
			_log.Log(LOG_STATUS, "OCTO_MQTT: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
		}
		subscribe(nullptr, m_TopicIn.c_str());
	}
	else {
		_log.Log(LOG_ERROR, "OCTO_MQTT: Connection failed!, restarting (rc=%d)", rc);
		m_bDoReconnect = true;
	}
}

void COctoPrintMQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!IsStopRequested(0))
		{
			if (rc == 5)
			{
				_log.Log(LOG_ERROR, "OCTO_MQTT: disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else
			{
				_log.Log(LOG_ERROR, "OCTO_MQTT: disconnected, restarting (rc=%d)", rc);
			}
			m_bDoReconnect = true;
		}
	}
}


bool COctoPrintMQTT::ConnectInt()
{
	StopMQTT();
	return ConnectIntEx();
}

bool COctoPrintMQTT::ConnectIntEx()
{
	m_bDoReconnect = false;
	_log.Log(LOG_STATUS, "OCTO_MQTT: Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 60;

	if (!m_CAFilename.empty()) {
		rc = tls_set(m_CAFilename.c_str());
		rc = tls_insecure_set(true);

		if (rc != MOSQ_ERR_SUCCESS)
		{
			_log.Log(LOG_ERROR, "OCTO_MQTT: Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		}
		_log.Log(LOG_STATUS, "OCTO_MQTT: enabled TLS mode");
	}
	rc = username_pw_set((!m_UserName.empty()) ? m_UserName.c_str() : nullptr, (!m_Password.empty()) ? m_Password.c_str() : nullptr);

	rc = connect(m_szIPAddress.c_str(), m_usIPPort, keepalive);
	if (rc != MOSQ_ERR_SUCCESS)
	{
		_log.Log(LOG_ERROR, "OCTO_MQTT: Failed to start, return code: %d (Check IP/Port)", rc);
		m_bDoReconnect = true;
		return false;
	}
	return true;
}

void COctoPrintMQTT::Do_Work()
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

	_log.Log(LOG_STATUS, "OCTO_MQTT: Worker stopped...");
}

void COctoPrintMQTT::SendHeartbeat()
{
	// not necessary for normal MQTT servers
}

void COctoPrintMQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	return; //not supported right now
	try {
		if (!m_IsConnected)
		{
			_log.Log(LOG_STATUS, "OCTO_MQTT: Not Connected, failed to send message: %s", Message.c_str());
			return;
		}
		publish(nullptr, Topic.c_str(), Message.size(), Message.c_str());
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "OCTO_MQTT: Failed to send message: %s", Message.c_str());
	}
}

void COctoPrintMQTT::WriteInt(const std::string &sendStr)
{
	if (sendStr.size() < 2)
		return;
	//string the return and the end
	//std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size());
	//SendMessage(m_TopicOut, sMessage);
}

void COctoPrintMQTT::UpdateUserVariable(const std::string &varName, const std::string &varValue)
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


void COctoPrintMQTT::on_message(const struct mosquitto_message *message)
{
	if (message->retain)
		return; //not interested in the last will
	std::string topic = message->topic;

	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

#ifdef DEBUG_OCTO_W
	SaveString2Disk(qMessage, "E:\\OCTO_mqtt.json");
#endif
#ifdef _DEBUG
	_log.Log(LOG_NORM, "OCTO_MQTT: Topic: %s", topic.c_str());
#else
	_log.Debug(DEBUG_HARDWARE, "OCTO_MQTT: Topic: %s", topic.c_str());
#endif
	if (qMessage.empty())
		return;

	///Get device name
	std::vector<std::string> strarray;
	StringSplit(topic, "/", strarray);
	if (strarray.size() < 2)
		return;

	std::string szMsgType = strarray[1];


	bool bIsJSON = (qMessage.find('{') == 0);

	if (bIsJSON)
	{
		Json::Value root;
		try {
			bool ret = ParseJSon(qMessage, root);
			if ((!ret) || (!root.isObject()))
			{
				_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid data received!");
				return;
			}

			if (root["_timestamp"].empty())
			{
				_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid data received! (no _timestamp field in JSON payload ?)");
				return;
			}

			if (szMsgType == "temperature")
			{
				if (strarray.size() < 3)
				{
					_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid temperature received!");
					return;
				}
				if (root["actual"].empty())
				{
					_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid temperature data received! (no actual field in JSON payload ?)");
					return;
				}
				std::string szSensorName = strarray[2];
				time_t atime = mytime(nullptr);
				if (m_LastSendTemp.find(szSensorName) != m_LastSendTemp.end())
				{
					time_t lastsend = m_LastSendTemp[szSensorName];
					if (atime - lastsend < 30)
						return; //do not update faster then 30 seconds
				}
				m_LastSendTemp[szSensorName] = atime;
				int crcID = Crc32(0, (const unsigned char*)szSensorName.c_str(), szSensorName.length());
				SendTempSensor(crcID, 255, std::stof(root["actual"].asString()), szSensorName);
			}
			else if (szMsgType == "progress")
			{
				if (strarray.size() < 3)
				{
					_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid event received!");
					return;
				}
				std::string szProgrssName = strarray[2];
				if (szProgrssName == "printing")
				{
					if (root["progress"].empty())
					{
						_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid progress data received! (no progress field in JSON payload ?)");
						return;
					}

					if (!root["printer_data"].empty())
					{
						//extended information
						Json::Value rootProgress = root["printer_data"];
						SendPercentageSensor(1, 1, 255, rootProgress["progress"]["completion"].asFloat(), "Printing Progress");
						if (!rootProgress["currentZ"].empty())
						{
							SendCustomSensor(1, 1, 255, rootProgress["currentZ"].asFloat(), "ZPos", "Z");
						}
						if (!rootProgress["progress"]["printTimeLeft"].isNull())
						{
							//in seconds
							int totSecondsLeft = rootProgress["progress"]["printTimeLeft"].asInt();
						}
					}
					else
						SendPercentageSensor(1, 1, 255, std::stof(root["progress"].asString()), "Printing Progress");

					if (!root["path"].empty())
					{
						SendTextSensor(TID_PATH, 1, 255, root["path"].asString(), "File Path");
					}

					//It is possible to enable extended data, this will be in a 'printer_data' object
					//for example:
					//	[printer_data][progress][completion] (percetage like 0.008484)
					//	[printer_data][progress][printTimeLeft] in seconds
				}
			}
			else if (szMsgType == "event")
			{
				if (strarray.size() < 3)
				{
					_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid event received!");
					return;
				}
				std::string szEventName = strarray[2];

				//There are many events, not all of them are handled (or need to be handled)
				//http://docs.octoprint.org/en/devel/events/index.html#available-events

				//Ignore the following events
				if (
					(szEventName == "CaptureStart")
					|| (szEventName == "CaptureDone")
					)
				{
					//we get these every xx seconds
					return;
				}

				UpdateUserVariable("LastEvent", szEventName);
				UpdateUserVariable("LastEventData", qMessage);


				if (szEventName != m_szLastEventName)
				{
					m_szLastEventName = szEventName;

					bool bIsPrintStatus(false);
					bool bIsPrinting(false);

					if (
						(szEventName == "PrintStarted")
						|| (szEventName == "PrintResumed")
						)
					{
						bIsPrintStatus = true;
						bIsPrinting = true;
					}
					else if (
						(szEventName == "PrintFailed")
						|| (szEventName == "PrintDone")
						|| (szEventName == "PrintCancelled")
						|| (szEventName == "PrintPaused")
						|| (szEventName == "PrintResumed")
						)
					{
						bIsPrintStatus = true;
						bIsPrinting = false;
					}
					else if (szEventName == "ZChange")
					{
						//Z-Position changed (new layer)
						if (root["new"].empty())
						{
							_log.Log(LOG_ERROR, "OCTO_MQTT: Invalid ZChange data received! (no new field in JSON payload ?)");
							return;
						}
						//SendCustomSensor(1, 1, 255, std::stof(root["new"].asString()), "ZChange", "Z");
						return;
					}
					if (bIsPrintStatus)
					{
						SendSwitch(1, 1, 255, bIsPrinting, 0, "Printing", m_Name);
						SendTextSensor(TID_PRINTING, 1, 255, szEventName, "Print Status");
					}
				}
			}
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "OCTO_MQTT: Error parsing message!!! (JSON Payload)");
			return;
		}
	}
	else {
		//Normal (Event) messages
		while (1 == 0);
	}
}
