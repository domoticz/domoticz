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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

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

	m_usIPPort=usIPPort;
	m_publish_topics = (_ePublishTopics)Topics;
	m_TopicIn = TOPIC_IN;
	m_TopicOut = TOPIC_OUT;
}

MQTT::~MQTT(void)
{
	mosqpp::lib_cleanup();
}

bool MQTT::StartHardware()
{
	RequestStart();

	//force connect the next first time
	m_IsConnected=false;

	m_bIsStarted = true;

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&MQTT::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());

	StartHeartbeatThread();
	return (m_thread != nullptr);
}

void MQTT::StopMQTT()
{
	disconnect();
	m_bIsStarted = false;
}

bool MQTT::StopHardware()
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
			_log.Log(LOG_STATUS, "MQTT: re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		} else {
			_log.Log(LOG_STATUS, "MQTT: connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
			m_sDeviceReceivedConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&MQTT::SendDeviceInfo, this, _1, _2, _3, _4));
			m_sSwitchSceneConnection = m_mainworker.sOnSwitchScene.connect(boost::bind(&MQTT::SendSceneInfo, this, _1, _2));
		}
		subscribe(NULL, m_TopicIn.c_str());
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

	if (topic != m_TopicIn)
		return;

	Json::Value root;
	Json::Reader jReader;
	std::string szCommand = "udevice";

	std::vector<std::vector<std::string> > result;
	
	uint64_t idx = 0;

	bool ret = jReader.parse(qMessage, root);
	if ((!ret) || (!root.isObject()))
		goto mqttinvaliddata;
	try
	{
		if (!root["command"].empty())
		{
			szCommand = root["command"].asString();
		}

		//Checks
		if ((szCommand == "udevice") || (szCommand == "switchlight") || (szCommand == "getdeviceinfo"))
		{
			idx = (uint64_t)root["idx"].asInt64();
			//Get the raw device parameters
			result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				_log.Log(LOG_ERROR, "MQTT: unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}
		else if ((szCommand == "switchscene") || (szCommand == "getsceneinfo"))
		{
			idx = (uint64_t)root["idx"].asInt64();
			result = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				_log.Log(LOG_ERROR, "MQTT: unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}
		else if (szCommand == "setuservariable")
		{
			idx = (uint64_t)root["idx"].asInt64();
			result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				_log.Log(LOG_ERROR, "MQTT: unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}
		
		//Perform Actions
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
			bool b_signallevel = ! root["RSSI"].empty();
			if (b_signallevel)
			{
				if (! root["RSSI"].isInt())
					goto mqttinvaliddata;
				signallevel = root["RSSI"].asInt();
			}

			int batterylevel = 255;
			bool b_batterylevel = ! root["Battery"].empty();
			if (b_batterylevel)
			{
				if (! root["Battery"].isInt())
					goto mqttinvaliddata;
				batterylevel = root["Battery"].asInt();
			}

			if (!m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, nvalue, svalue, signallevel, batterylevel, bParseTrigger))
			{
				_log.Log(LOG_ERROR, "MQTT: Problem updating sensor (check idx, hardware enabled)");
				return;
			}
			return;
		}
		else if (szCommand == "switchlight")
		{
			std::string switchcmd = root["switchcmd"].asString();
			//if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle") && (switchcmd != "Set Level") && (switchcmd != "Stop"))
			//	goto mqttinvaliddata;
			int level = 0;
			if (!root["level"].empty())
			{
				if (root["level"].isString())
					level = atoi(root["level"].asString().c_str());
				else
					level = root["level"].asInt();
			}
			if (!m_mainworker.SwitchLight(idx, switchcmd, level, NoColor, false, 0) == true)
			{
				_log.Log(LOG_ERROR, "MQTT: Error sending switch command!");
			}
		}
		else if (szCommand == "setcolbrightnessvalue")
		{
			idx = (uint64_t)root["idx"].asInt64();

			if (root["switchcmd"].empty())
				root["switchcmd"] = "On";

			std::string switchcmd = root["switchcmd"].asString();
			if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle") && (switchcmd != "Set Level") && (switchcmd != "Set Color"))
				goto mqttinvaliddata;

			_tColor color;

			std::string hex = root["hex"].asString();
			std::string hue = root["hue"].asString();
			std::string sat = root["sat"].asString();
			std::string brightness = root["level"].asString();
			std::string iswhite;
			if (!root["isWhite"].empty())
			{
				if (root["isWhite"].isString())
					iswhite = root["isWhite"].asString();
				else
				{
					iswhite = root["isWhite"].asInt()!=0?"true":"false";
				}
			}

			int ival = 100;
			float brightnessAdj = 1.0f;

			if (!root["color"].empty())
			{
				color = _tColor(root["color"]);
				if (color.mode == ColorModeRGB)
				{
					// Normalize RGB to full brightness
					float hsb[3];
					int r, g, b;
					rgb2hsb(color.r, color.g, color.b, hsb);
					hsb2rgb(hsb[0]*360.0f, hsb[1], 1.0f, r, g, b, 255);
					color.r = r;
					color.g = g;
					color.b = b;
					brightnessAdj = hsb[2];
				}
				//_log.Debug(DEBUG_NORM, "MQTT: setcolbrightnessvalue: color: '%s', bri: '%s'", color.toString().c_str(), brightness.c_str());
			}
			else if (!hex.empty())
			{
				uint64_t ihex = hexstrtoui64(hex);
				//_log.Debug(DEBUG_NORM, "MQTT: setcolbrightnessvalue: hex: '%s', ihex: %" PRIx64 ", bri: '%s', iswhite: '%s'", hex.c_str(), ihex, brightness.c_str(), iswhite.c_str());
				uint8_t r = 0;
				uint8_t g = 0;
				uint8_t b = 0;
				uint8_t cw = 0;
				uint8_t ww = 0;
				switch (hex.length())
				{
					case 6: //RGB
						r = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						g = (uint8_t)((ihex & 0x000000FF00) >> 8);
						b = (uint8_t)ihex & 0xFF;
						float hsb[3];
						int tr, tg, tb; // tmp of 'int' type so can be passed as references to hsb2rgb
						rgb2hsb(r, g, b, hsb);
						// Normalize RGB to full brightness
						hsb2rgb(hsb[0]*360.0f, hsb[1], 1.0f, tr, tg, tb, 255);
						r = tr;
						g = tg;
						b = tb;
						brightnessAdj = hsb[2];
						// Backwards compatibility: set iswhite for unsaturated colors
						iswhite = (hsb[1] < (20.0 / 255.0)) ? "true" : "false";
						color = _tColor(r, g, b, cw, ww, ColorModeRGB);
						break;
					case 8: //RGB_WW
						r = (uint8_t)((ihex & 0x00FF000000) >> 24);
						g = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						b = (uint8_t)((ihex & 0x000000FF00) >> 8);
						ww = (uint8_t)ihex & 0xFF;
						color = _tColor(r, g, b, cw, ww, ColorModeCustom);
						break;
					case 10: //RGB_CW_WW
						r = (uint8_t)((ihex & 0xFF00000000) >> 32);
						g = (uint8_t)((ihex & 0x00FF000000) >> 24);
						b = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						cw = (uint8_t)((ihex & 0x000000FF00) >> 8);
						ww = (uint8_t)ihex & 0xFF;
						color = _tColor(r, g, b, cw, ww, ColorModeCustom);
						break;
				}
				if (iswhite == "true") color.mode = ColorModeWhite;
				//_log.Debug(DEBUG_NORM, "MQTT: setcolbrightnessvalue: trgbww: %02x%02x%02x%02x%02x, color: '%s'", r, g, b, cw, ww, color.toString().c_str());
			}
			else if (!hue.empty())
			{
				int r, g, b;

				//convert hue to RGB
				float iHue = float(atof(hue.c_str()));
				float iSat = 100.0f;
				if (!sat.empty()) iSat = float(atof(sat.c_str()));
				hsb2rgb(iHue, iSat/100.0f, 1.0f, r, g, b, 255);

				color = _tColor(r, g, b, 0, 0, ColorModeRGB);
				if (iswhite == "true") color.mode = ColorModeWhite;
				//_log.Debug(DEBUG_NORM, "MQTT: setcolbrightnessvalue2: hue: %f, rgb: %02x%02x%02x, color: '%s'", iHue, r, g, b, color.toString().c_str());
			}

			if (color.mode == ColorModeNone)
			{
				goto mqttinvaliddata;
			}

			if (!brightness.empty())
				ival = atoi(brightness.c_str());
			ival = int(ival * brightnessAdj);
			ival = std::max(ival, 0);
			ival = std::min(ival, 100);

			_log.Log(LOG_STATUS, "MQTT: setcolbrightnessvalue: ID: %" PRIx64 ", bri: %d, color: '%s'", idx, ival, color.toString().c_str());

			if (!m_mainworker.SwitchLight(idx, switchcmd, ival, color, false, 0) == true)
			{
				_log.Log(LOG_ERROR, "MQTT: Error sending switch command!");
			}
		}
		else if (szCommand == "switchscene")
		{
			std::string switchcmd = root["switchcmd"].asString();
			if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle"))
				goto mqttinvaliddata;
			if (!m_mainworker.SwitchScene(idx, switchcmd) == true)
			{
				_log.Log(LOG_ERROR, "MQTT: Error sending scene command!");
			}
		}
		else if (szCommand == "setuservariable")
		{
			std::string varvalue = root["value"].asString();

			idx = (uint64_t)root["idx"].asInt64();
			result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID==%" PRIu64 ")", idx);
			std::string sVarName = result[0][0];
			_eUsrVariableType varType = (_eUsrVariableType)atoi(result[0][1].c_str());

			std::string errorMessage;
			if (!m_sql.UpdateUserVariable(root["idx"].asString(), sVarName, varType, varvalue, true, errorMessage))
			{
				_log.Log(LOG_ERROR, "MQTT: Error setting uservariable (%s)", errorMessage.c_str());
			}
		}
		else if (szCommand == "addlogmessage")
		{
			std::string msg = root["message"].asString();
			_log.Log(LOG_STATUS, "MQTT MSG: %s", msg.c_str());
		}
		else if (szCommand == "sendnotification")
		{
			std::string subject, body, sound;
			int priority = 0;
			if (!root["subject"].empty())
			{
				subject = root["subject"].asString();
			}
			if (!root["body"].empty())
			{
				body = root["body"].asString();
			}
			if (!root["priority"].empty())
			{
				priority = root["priority"].asInt();
			}
			if (!root["sound"].empty())
			{
				sound = root["sound"].asString();
			}
			m_notifications.SendMessageEx(0, std::string(""), NOTIFYALL, subject, body, std::string(""), priority, sound, true);
		}
		else if (szCommand == "getdeviceinfo")
		{
			int HardwareID = atoi(result[0][0].c_str());
			SendDeviceInfo(HardwareID, idx, "request device", NULL);
		}
		else if (szCommand == "getsceneinfo")
		{
			SendSceneInfo(idx, "request scene");
		}
		else
		{
			_log.Log(LOG_ERROR, "MQTT: Unknown command received: %s", szCommand.c_str());
			return;
		}
		return;
	}
	catch (const Json::LogicError&)
	{
		goto mqttinvaliddata;
	}
mqttinvaliddata:
	_log.Log(LOG_ERROR, "MQTT: Invalid data received!");
}

void MQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!IsStopRequested(0))
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
				if (isConnected() && sec_counter % 10 == 0)
				{
					SendHeartbeat();
				}
			}
		}
	}
	if (m_sDeviceReceivedConnection.connected())
		m_sDeviceReceivedConnection.disconnect();
	if (m_sSwitchSceneConnection.connected())
		m_sSwitchSceneConnection.disconnect();

	_log.Log(LOG_STATUS,"MQTT: Worker stopped...");
}

void MQTT::SendHeartbeat()
{
	// not necessary for normal MQTT servers
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
	if (sendStr.size() < 2)
		return;
	//string the return and the end
	std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size());
	SendMessage(m_TopicOut, sMessage);
}

void MQTT::SendDeviceInfo(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	if (!m_IsConnected)
		return;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT DeviceID, Unit, Name, [Type], SubType, nValue, sValue, SwitchType, SignalLevel, BatteryLevel, Options, Description, LastLevel, Color FROM DeviceStatus WHERE (HardwareID==%d) AND (ID==%" PRIu64 ")", m_HwdID, DeviceRowIdx);
	if (!result.empty())
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
		std::string description = sd[11];
		int LastLevel = atoi(sd[12].c_str());
		std::string sColor = sd[13];

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
		else if ((dType == pTypeRFXMeter) || (dType == pTypeRFXSensor)) {
			root["meterType"] = Meter_Type_Desc((_eMeterType)switchType);
		}
		// Add device options
		for (const auto & ittOptions : options)
		{
			std::string optionName = ittOptions.first;
			std::string optionValue = ittOptions.second;
			root[optionName] = optionValue;
		}

		root["RSSI"] = RSSI;
		root["Battery"] = BatteryLevel;
		root["nvalue"] = nvalue;
		root["description"] = description;

		if (switchType == STYPE_Dimmer)
		{
			root["Level"] = LastLevel;
			if (dType == pTypeColorSwitch)
			{
				_tColor color(sColor);
				root["Color"] = color.toJSONValue();
			}
		}

		//give all svalues separate
		std::vector<std::string> strarray;
		StringSplit(svalue, ";", strarray);

		int sIndex = 1;
		for (const auto & itt : strarray)
		{
			std::stringstream szQuery;
			szQuery << "svalue" << sIndex;
			root[szQuery.str()] = itt;
			sIndex++;
		}
		std::string message =  root.toStyledString();
		if (m_publish_topics & PT_out)
		{
			SendMessage(TOPIC_OUT, message);
		}

		if (m_publish_topics & PT_floor_room) {
			result = m_sql.safe_query("SELECT F.Name, P.Name, M.DeviceRowID FROM Plans as P, Floorplans as F, DeviceToPlansMap as M WHERE P.FloorplanID=F.ID and M.PlanID=P.ID and M.DeviceRowID=='%" PRIu64 "'", DeviceRowIdx);
			for(size_t i=0 ; i<result.size(); i++)
			{
				std::vector<std::string> sd = result[i];
				std::string floor = sd[0];
				std::string room =  sd[1];
				std::stringstream topic;
				topic << TOPIC_OUT << "/" << floor << "/" + room;

				SendMessage(topic.str() , message);
			}
		}
	}
}

void MQTT::SendSceneInfo(const uint64_t SceneIdx, const std::string &SceneName)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes WHERE (ID==%" PRIu64 ") ORDER BY [Order]", SceneIdx);
	if (result.empty())
		return;
	std::vector<std::string> sd = result[0];

	std::string sName = sd[1];
	std::string sLastUpdate = sd[6].c_str();

	unsigned char nValue = atoi(sd[4].c_str());
	unsigned char scenetype = atoi(sd[5].c_str());
	//int iProtected = atoi(sd[7].c_str());

	//std::string onaction = base64_encode((sd[8]);
	//std::string offaction = base64_encode(sd[9]);

	Json::Value root;

	root["idx"] = atoi(sd[0].c_str());
	root["Name"] = sName;
	//root["Description"] = sd[10];
	//root["Favorite"] = atoi(sd[3].c_str());
	//root["Protected"] = (iProtected != 0);
	//root["OnAction"] = onaction;
	//root["OffAction"] = offaction;

	if (scenetype == 0)
	{
		root["Type"] = "Scene";
	}
	else
	{
		root["Type"] = "Group";
	}

	root["LastUpdate"] = sLastUpdate;

	if (nValue == 0)
		root["Status"] = "Off";
	else if (nValue == 1)
		root["Status"] = "On";
	else
		root["Status"] = "Mixed";
	root["Timers"] = (m_sql.HasSceneTimers(sd[0]) == true) ? "true" : "false";
/*
	uint64_t camIDX = m_mainworker.m_cameras.IsDevSceneInCamera(1, sd[0]);
	//root["UsedByCamera"] = (camIDX != 0) ? true : false;
	if (camIDX != 0) {
		std::stringstream scidx;
		scidx << camIDX;
		//root["CameraIdx"] = std::to_string(camIDX);
	}
*/
	std::string message = root.toStyledString();
	if (m_publish_topics & PT_out)
	{
		SendMessage(TOPIC_OUT, message);
	}
}
