#include "stdafx.h"
#include "MQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../json/json.h"
#include "../notifications/NotificationHelper.h"

#define RETRY_DELAY 30

#define CLIENTID	"Domoticz"
#define TOPIC_OUT	"domoticz/out"
#define TOPIC_IN	"domoticz/in"
#define QOS         1

MQTT::MQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilename, const int Topics) :
m_szIPAddress(IPAddress),
m_UserName(Username),
m_Password(Password),
m_CAFilename(CAfilename)
{
	m_HwdID=ID;
	m_IsConnected = false;
	m_bDoReconnect = false;
	mosqpp::lib_init();

	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_publish_topics = (_ePublishTopics)Topics;
}

MQTT::~MQTT(void)
{
	mosqpp::lib_cleanup();
}

bool MQTT::StartHardware()
{
	StartHeartbeatThread();

	m_stoprequested=false;

	//force connect the next first time
	m_IsConnected=false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MQTT::Do_Work, this)));
	return (m_thread!=NULL);
}

void MQTT::StopMQTT()
{
	disconnect();
	m_bIsStarted = false;
}

bool MQTT::StopHardware()
{
	StopHeartbeatThread();
	m_stoprequested=true;
	try {
		if (m_thread)
		{
			m_thread->join();
			m_thread.reset();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	if (m_sConnection.connected())
		m_sConnection.disconnect();
	m_IsConnected = false;
	return true;
}

void MQTT::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	_log.Log(LOG_STATUS, "MQTT: Subscribed");
	m_IsConnected = true;
}

void MQTT::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	if (rc == 0){
		if (m_IsConnected) {
			_log.Log(LOG_STATUS, "MQTT: re-connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		} else {
			_log.Log(LOG_STATUS, "MQTT: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
			m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&MQTT::SendDeviceInfo, this, _1, _2, _3, _4));
		}
		subscribe(NULL, TOPIC_IN);
	}
	else {
		_log.Log(LOG_ERROR, "MQTT: Connection failed!, restarting (rc=%d)",rc);
		m_bDoReconnect = true;
	}
}

void MQTT::on_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	_log.Log(LOG_NORM, "MQTT: Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());

	if (qMessage.empty())
		return;

	if (topic.find("MyMQTT") != std::string::npos)
	{
		//MySensors message
		ProcessMySensorsMessage(qMessage);
		return;
	}
	else if (topic != TOPIC_IN)
		return;
	Json::Value root;
	Json::Reader jReader;
	std::string szCommand = "udevice";
	std::vector<std::vector<std::string> > result;
	unsigned long long idx = 0;

	bool ret = jReader.parse(qMessage, root);
	if (!ret)
		goto mqttinvaliddata;


	if (!root["command"].empty())
	{
		if (!root["command"].isString())
			goto mqttinvaliddata;
		szCommand = root["command"].asString();
	}

	if ((szCommand == "udevice") || (szCommand == "switchlight") || (szCommand == "getdeviceinfo"))
	{
		if (root["idx"].empty())
			goto mqttinvaliddata;
		if (!root["idx"].isInt64())
			goto mqttinvaliddata;

		idx = (unsigned long long)root["idx"].asInt64();
		//Get the raw device parameters
		result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%llu)", idx);
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "MQTT: unknown idx received!");
			return;
		}
	}
	else if (szCommand == "switchscene")
	{
		if (root["idx"].empty())
			goto mqttinvaliddata;
		if (!root["idx"].isInt64())
			goto mqttinvaliddata;

		idx = (unsigned long long)root["idx"].asInt64();
		result = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID==%llu)", idx);
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "MQTT: unknown idx received!");
			return;
		}
	}
	else if (szCommand == "setuservariable")
	{
		if (root["idx"].empty())
			goto mqttinvaliddata;
		if (!root["idx"].isInt64())
			goto mqttinvaliddata;

		idx = (unsigned long long)root["idx"].asInt64();
		result = m_sql.safe_query("SELECT Name FROM UserVariables WHERE (ID==%llu)", idx);
		if (result.empty())
		{
			_log.Log(LOG_ERROR, "MQTT: unknown idx received!");
			return;
		}
	}

	if (szCommand == "udevice")
	{
		int HardwareID = atoi(result[0][0].c_str());
		std::string DeviceID = result[0][1];
		int unit = atoi(result[0][2].c_str());
		int devType = atoi(result[0][3].c_str());
		int subType = atoi(result[0][4].c_str());

		bool bnvalue = !root["nvalue"].empty();
		bool bsvalue = !root["svalue"].empty();
		bool bParseValue = !root["parse"].empty();

		if (!bnvalue && !bsvalue)
			goto mqttinvaliddata;

		if (bnvalue)
		{
			if (!root["nvalue"].isInt())
				goto mqttinvaliddata;
		}
		if (bsvalue)
		{
			if (!root["svalue"].isString())
				goto mqttinvaliddata;
		}

		int nvalue = (bnvalue) ? root["nvalue"].asInt() : 0;
		std::string svalue = (bsvalue) ? root["svalue"].asString() : "";
		bool bParseTrigger = (bParseValue) ? root["parse"].asBool() : true;

		int signallevel = 12;
		int batterylevel = 255;

		if (!m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, nvalue, svalue, signallevel, batterylevel, bParseTrigger))
		{
			_log.Log(LOG_ERROR, "MQTT: Problem updating sensor (check idx, hardware enabled)");
			return;
		}
		return;
	}
	else if (szCommand == "switchlight")
	{
		if (root["switchcmd"].empty())
			goto mqttinvaliddata;
		if (!root["switchcmd"].isString())
			goto mqttinvaliddata;
		std::string switchcmd = root["switchcmd"].asString();
		if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle") && (switchcmd != "Set Level"))
			goto mqttinvaliddata;
		int level = 0;
		if (!root["level"].empty())
		{
			if (root["level"].isString())
				level = atoi(root["level"].asString().c_str());
			else
				level = root["level"].asInt();
		}
		if (!m_mainworker.SwitchLight(idx, switchcmd, level, -1, false, 0) == true)
		{
			_log.Log(LOG_ERROR, "MQTT: Error sending switch command!");
		}
		return;
	}
	else if (szCommand == "switchscene")
	{
		if (root["switchcmd"].empty())
			goto mqttinvaliddata;
		if (!root["switchcmd"].isString())
			goto mqttinvaliddata;
		std::string switchcmd = root["switchcmd"].asString();
		if ((switchcmd != "On") && (switchcmd != "Off"))
			goto mqttinvaliddata;
		if (!m_mainworker.SwitchScene(idx, switchcmd) == true)
		{
			_log.Log(LOG_ERROR, "MQTT: Error sending scene command!");
		}
		return;
	}
	else if (szCommand == "setuservariable")
	{
		if (root["value"].empty())
			goto mqttinvaliddata;
		if (!root["value"].isString())
			goto mqttinvaliddata;
		std::string varvalue = root["value"].asString();
		m_sql.SetUserVariable(idx, varvalue, true);
		return;
	}
	else if (szCommand == "addlogmessage")
	{
		if (root["message"].empty())
			goto mqttinvaliddata;
		if (!root["message"].isString())
			goto mqttinvaliddata;
		std::string msg = root["message"].asString();
		_log.Log(LOG_STATUS, "MQTT MSG: %s", msg.c_str());
		return;
	}
	else if (szCommand == "sendnotification")
	{
		std::string subject(""), body(""), sound("");
		int priority = 0;
		if (!root["subject"].empty())
		{
			if (!root["subject"].isString())
				goto mqttinvaliddata;
			subject = root["subject"].asString();
		}
		if (!root["body"].empty())
		{
			if (!root["body"].isString())
				goto mqttinvaliddata;
			body = root["body"].asString();
		}
		if (!root["priority"].empty())
		{
			if (!root["priority"].isInt())
				goto mqttinvaliddata;
			priority = root["priority"].asInt();
		}
		if (!root["sound"].empty())
		{
			if (!root["sound"].isString())
				goto mqttinvaliddata;
			sound = root["sound"].asString();
		}
		m_notifications.SendMessageEx(NOTIFYALL, subject, body, "", priority, sound, true);
		std::string varvalue = root["value"].asString();
		m_sql.SetUserVariable(idx, varvalue, true);
		return;
	}
	else if (szCommand == "getdeviceinfo")
	{
		int HardwareID = atoi(result[0][0].c_str());
		SendDeviceInfo(HardwareID, idx, "request device", NULL);
		return;
	}
	else
	{
		_log.Log(LOG_ERROR, "MQTT: Unknown command received: %s", szCommand.c_str());
		return;
	}
mqttinvaliddata:
	_log.Log(LOG_ERROR, "MQTT: Invalid data received!");
}

void MQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!m_stoprequested)
		{
			if (rc == 5)
			{
				_log.Log(LOG_ERROR, "MQTT: disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else
			{
				_log.Log(LOG_ERROR, "MQTT: disconnected, restarting (rc=%d)", rc);
			}
			m_bDoReconnect = true;
		}
	}
}


bool MQTT::ConnectInt()
{
	StopMQTT();
	return ConnectIntEx();
}

bool MQTT::ConnectIntEx()
{
	m_bDoReconnect = false;
	_log.Log(LOG_STATUS, "MQTT: Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 60;

	if (!m_CAFilename.empty()){
		rc = tls_set(m_CAFilename.c_str());

		if ( rc != MOSQ_ERR_SUCCESS)
		{
			_log.Log(LOG_ERROR, "MQTT: Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		} else {
			_log.Log(LOG_STATUS, "MQTT: enabled TLS mode");
		}
	}
	rc = username_pw_set((!m_UserName.empty()) ? m_UserName.c_str() : NULL, (!m_Password.empty()) ? m_Password.c_str() : NULL);

	rc = connect(m_szIPAddress.c_str(), m_usIPPort, keepalive);
	if ( rc != MOSQ_ERR_SUCCESS)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to start, return code: %d (Check IP/Port)", rc);
		m_bDoReconnect = true;
		return false;
	}
	return true;
}

void MQTT::Do_Work()
{
	bool bFirstTime=true;
	int msec_counter = 0;
	int sec_counter = 0;

	while (!m_stoprequested)
	{
		sleep_milliseconds(100);
		if (!bFirstTime)
		{
			int rc = loop();
			if (rc) {
				if (rc != MOSQ_ERR_NO_CONN)
				{
					if (!m_stoprequested)
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
				m_LastHeartbeat=mytime(NULL);
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
			}
		}
	}
	_log.Log(LOG_STATUS,"MQTT: Worker stopped...");
}

void MQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	try {
		if (!m_IsConnected)
		{
			_log.Log(LOG_STATUS, "MQTT: Not Connected, failed to send message: %s", Message.c_str());
			return;
		}
		publish(NULL, Topic.c_str(), Message.size(), Message.c_str());
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "MQTT: Failed to send message: %s", Message.c_str());
	}
}

void MQTT::WriteInt(const std::string &sendStr)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	if (sendStr.size() < 2)
		return;
	//string the return and the end
	std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size()-1);
	SendMessage("MyMQTT", sMessage);
}

void MQTT::ProcessMySensorsMessage(const std::string &MySensorsMessage)
{
	m_bufferpos = MySensorsMessage.size();
	memcpy(&m_buffer, MySensorsMessage.c_str(), m_bufferpos);
	m_buffer[m_bufferpos] = 0;
	ParseLine();
}

void MQTT::SendDeviceInfo(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	boost::lock_guard<boost::mutex> l(m_mqtt_mutex);
	if (!m_IsConnected)
		return;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT DeviceID, Unit, Name, [Type], SubType, nValue, sValue, SwitchType, SignalLevel, BatteryLevel, Options FROM DeviceStatus WHERE (HardwareID==%d) AND (ID==%llu)", m_HwdID, DeviceRowIdx);
	if (result.size() > 0)
	{
		std::vector<std::string> sd = result[0];
		std::string did = sd[0];
		int dunit = atoi(sd[1].c_str());
		std::string name = sd[2];
		int dType = atoi(sd[3].c_str());
		int dSubType = atoi(sd[4].c_str());
		int nvalue = atoi(sd[5].c_str());
		std::string svalue = sd[6];
		_eSwitchType switchType = (_eSwitchType)atoi(sd[7].c_str());
		int RSSI = atoi(sd[8].c_str());
		int BatteryLevel = atoi(sd[9].c_str());
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[10]);

		Json::Value root;

		root["idx"] = DeviceRowIdx;
		root["id"] = did;
		root["unit"] = dunit;
		root["name"] = name;
		root["dtype"] = RFX_Type_Desc(dType,1);
		root["stype"] = RFX_Type_SubType_Desc(dType, dSubType);

		if (IsLightOrSwitch(dType, dSubType) == true) {
			root["switchType"] = Switch_Type_Desc(switchType);
		}
		// Add device options
		std::map<std::string, std::string>::const_iterator ittOptions;
		for (ittOptions = options.begin(); ittOptions != options.end(); ++ittOptions)
		{
			std::string optionName = ittOptions->first.c_str();
			std::string optionValue = ittOptions->second.c_str();
			root[optionName] = optionValue;
		}

		root["RSSI"] = RSSI;
		root["Battery"] = BatteryLevel;
		root["nvalue"] = nvalue;

		//give all svalues separate
		std::vector<std::string> strarray;
		StringSplit(svalue, ";", strarray);

		std::vector<std::string>::const_iterator itt;
		int sIndex = 1;
		for (itt = strarray.begin(); itt != strarray.end(); ++itt)
		{
			std::stringstream szQuery("");
			szQuery << "svalue" << sIndex;
			root[szQuery.str()] = *itt;
			sIndex++;
		}
		std::string message =  root.toStyledString();
		if (m_publish_topics & PT_out)
		{
			SendMessage(TOPIC_OUT, message);
		}

		if (m_publish_topics & PT_floor_room) {
			result = m_sql.safe_query("SELECT F.Name, P.Name, M.DeviceRowID FROM Plans as P, Floorplans as F, DeviceToPlansMap as M WHERE P.FloorplanID=F.ID and M.PlanID=P.ID and M.DeviceRowID=='%llu'", DeviceRowIdx);
			for(size_t i=0 ; i<result.size(); i++)
			{
				std::vector<std::string> sd = result[i];
				std::string floor = sd[0];
				std::string room =  sd[1];
				std::stringstream topic("");
				topic << TOPIC_OUT << "/" << floor << "/" + room;

				SendMessage(topic.str() , message);
			}
		}
	}
}

