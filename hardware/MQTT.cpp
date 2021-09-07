#include "stdafx.h"
#include "MQTT.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/json_helper.h"
#include "../notifications/NotificationHelper.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define RETRY_DELAY 30

#define CLIENTID "Domoticz"
#define TOPIC_OUT "domoticz/out"
#define TOPIC_IN "domoticz/in"
#define QOS 1
#define RETAIN_BIT 0x80

namespace
{
	constexpr std::array<const char *, 3> szTLSVersions{
		"tlsv1",   //
		"tlsv1.1", //
		"tlsv1.2", //
	};
} // namespace

MQTT::MQTT(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &Username, const std::string &Password, const std::string &CAfilenameExtra,
	   const int TLS_Version, const int PublishScheme, const std::string &MQTTClientID, const bool PreventLoop)
	: mosqdz::mosquittodz(MQTTClientID.c_str())
	, m_szIPAddress(IPAddress)
	, m_UserName(Username)
	, m_Password(Password)
	, m_CAFilename(CAfilenameExtra)
{
	m_HwdID = ID;
	m_IsConnected = false;
	m_bDoReconnect = false;
	mosqdz::lib_init();

	m_usIPPort = usIPPort;
	m_bRetain = (PublishScheme & RETAIN_BIT);
	m_publish_scheme = (_ePublishTopics)(PublishScheme & ~RETAIN_BIT);
	Debug(DEBUG_HARDWARE, "MQTT PublishSchema %d (%d), Retain %d", m_publish_scheme, PublishScheme, m_bRetain);

	std::vector<std::string> strarray;
	StringSplit(CAfilenameExtra, ";", strarray);
	if (!strarray.empty())
	{
		m_CAFilename = strarray[0];

		if (strarray.size() > 1)
		{
			if (!strarray[1].empty())
				m_TopicIn = strarray[1];
		}
		if (strarray.size() > 2)
		{
			if (!strarray[2].empty())
				m_TopicOut = strarray[2];
		}
		if (strarray.size() > 3)
		{
			m_TopicDiscoveryPrefix = strarray[3];
		}
	}

	m_TLS_Version = (TLS_Version < 3) ? TLS_Version : 0; // see szTLSVersions

	m_bPreventLoop = PreventLoop;

	threaded_set(true);
}

MQTT::~MQTT()
{
	mosqdz::lib_cleanup();
}

bool MQTT::StartHardware()
{
	RequestStart();

	// force connect the next first time
	m_IsConnected = false;
	m_bIsStarted = true;

	m_LastUpdatedDeviceRowIdx = 0;
	m_LastUpdatedSceneRowIdx = 0;

	// Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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

void MQTT::on_subscribe(int /*mid*/, int /*qos_count*/, const int * /*granted_qos*/)
{
	//Log(LOG_STATUS, "Subscribed");
	m_IsConnected = true;
}

void MQTT::on_log(int level, const char *str)
{
	if (level & MOSQ_LOG_DEBUG)
		return;
	_eLogLevel llevel = LOG_NORM;
	switch (level)
	{
		case MOSQ_LOG_NOTICE:
			llevel = LOG_STATUS;
			break;
		default:
			llevel = LOG_ERROR;
	}
	Log(llevel, "%s", str);
}

void MQTT::on_error()
{
}

void MQTT::on_connect(int rc)
{
	/* rc=
	** 0 - success
	** 1 - connection refused(unacceptable protocol version)
	** 2 - connection refused(identifier rejected)
	** 3 - connection refused(broker unavailable)
	*/

	if (rc == 0)
	{
		if (m_IsConnected)
		{
			Log(LOG_STATUS, "re-connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
		}
		else
		{
			Log(LOG_STATUS, "connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
			m_IsConnected = true;
			sOnConnected(this);
			m_sDeviceReceivedConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, auto &&name, auto cmd) { SendDeviceInfo(id, idx, name, cmd); });
			m_sSwitchSceneConnection = m_mainworker.sOnSwitchScene.connect([this](auto scene, auto &&name) { SendSceneInfo(scene, name); });
		}
		if (!m_TopicIn.empty())
			SubscribeTopic(m_TopicIn.c_str());
		else
			Log(LOG_STATUS, "Default input topic disabled in settings...");

		if (!m_TopicDiscoveryPrefix.empty())
		{
			SubscribeTopic((m_TopicDiscoveryPrefix + std::string("/#")).c_str());
			//Send online status
			SendMessage(m_TopicDiscoveryPrefix + std::string("/status"), "online");
		}
	}
	else
	{
		Log(LOG_ERROR, "Connection failed!, restarting (rc=%d)", rc);
		m_bDoReconnect = true;
	}
}

void MQTT::on_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char *)message->payload, (char *)message->payload + message->payloadlen);

	Debug(DEBUG_HARDWARE, "Topic: %s, Message: %s", topic.c_str(), qMessage.c_str());

	if (qMessage.empty())
		return;

	if (topic != m_TopicIn)
	{
		if (topic.find(m_TopicDiscoveryPrefix) == 0)
		{
			on_auto_discovery_message(message);
			return;
		}
		if (m_subscribed_topics.find(topic)!=m_subscribed_topics.end())
		{
			handle_auto_discovery_sensor_message(message);
			return;
		}
		return;
	}

	Json::Value root;
	std::string szCommand = "udevice";

	std::vector<std::vector<std::string>> result;

	uint64_t idx = 0;

	bool ret = ParseJSon(qMessage, root);
	if ((!ret) || (!root.isObject()))
		goto mqttinvaliddata;
	try
	{
		if (!root["command"].empty())
		{
			szCommand = root["command"].asString();
		}

		// Checks
		if ((szCommand == "udevice") || (szCommand == "switchlight") || (szCommand == "getdeviceinfo"))
		{
			idx = (uint64_t)root["idx"].asInt64();
			// Get the raw device parameters
			result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				Log(LOG_ERROR, "unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}
		else if ((szCommand == "switchscene") || (szCommand == "getsceneinfo"))
		{
			idx = (uint64_t)root["idx"].asInt64();
			result = m_sql.safe_query("SELECT Name FROM Scenes WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				Log(LOG_ERROR, "unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}
		else if (szCommand == "setuservariable")
		{
			idx = (uint64_t)root["idx"].asInt64();
			result = m_sql.safe_query("SELECT Name, ValueType FROM UserVariables WHERE (ID==%" PRIu64 ")", idx);
			if (result.empty())
			{
				Log(LOG_ERROR, "unknown idx received! (idx %" PRIu64 ")", idx);
				return;
			}
		}

		// Perform Actions
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
			bool b_signallevel = !root["RSSI"].empty();
			if (b_signallevel)
			{
				if (!root["RSSI"].isInt())
					goto mqttinvaliddata;
				signallevel = root["RSSI"].asInt();
			}

			int batterylevel = 255;
			bool b_batterylevel = !root["Battery"].empty();
			if (b_batterylevel)
			{
				if (!root["Battery"].isInt())
					goto mqttinvaliddata;
				batterylevel = root["Battery"].asInt();
			}

			// Prevent MQTT update being send to client after next update
			m_LastUpdatedDeviceRowIdx = idx;

			if (!m_mainworker.UpdateDevice(HardwareID, DeviceID, unit, devType, subType, nvalue, svalue, m_Name, signallevel, batterylevel, bParseTrigger))
			{
				Log(LOG_ERROR, "Problem updating sensor (check idx, hardware enabled)");
				return;
			}
			return;
		}
		if (szCommand == "switchlight")
		{
			std::string switchcmd = root["switchcmd"].asString();
			// if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle") && (switchcmd != "Set Level") && (switchcmd != "Stop"))
			//	goto mqttinvaliddata;
			int level = 0;
			if (!root["level"].empty())
			{
				if (root["level"].isString())
					level = atoi(root["level"].asString().c_str());
				else
					level = root["level"].asInt();
			}

			// Prevent MQTT update being send to client after next update
			m_LastUpdatedDeviceRowIdx = idx;

			if (!m_mainworker.SwitchLight(idx, switchcmd, level, NoColor, false, 0, "MQTT") == true)
			{
				Log(LOG_ERROR, "Error sending switch command!");
			}
		}
		else if (szCommand == "setcolbrightnessvalue")
		{
			idx = (uint64_t)root["idx"].asInt64();
			_tColor color;

			std::string hex = root["hex"].asString();
			std::string hue = root["hue"].asString();
			std::string sat = root["sat"].asString();
			std::string brightness = root["brightness"].asString();
			std::string iswhite;
			if (!root["isWhite"].empty())
			{
				if (root["isWhite"].isString())
					iswhite = root["isWhite"].asString();
				else
				{
					iswhite = root["isWhite"].asInt() != 0 ? "true" : "false";
				}
			}

			int ival = 100;
			float brightnessAdj = 1.0F;

			if (!root["color"].empty())
			{
				color = _tColor(root["color"]);
				if (color.mode == ColorModeRGB)
				{
					// Normalize RGB to full brightness
					float hsb[3];
					int r, g, b;
					rgb2hsb(color.r, color.g, color.b, hsb);
					hsb2rgb(hsb[0] * 360.0F, hsb[1], 1.0F, r, g, b, 255);
					color.r = (uint8_t)r;
					color.g = (uint8_t)g;
					color.b = (uint8_t)b;
					brightnessAdj = hsb[2];
				}
				// Debug(DEBUG_NORM, "setcolbrightnessvalue: color: '%s', bri: '%s'", color.toString().c_str(), brightness.c_str());
			}
			else if (!hex.empty())
			{
				uint64_t ihex = hexstrtoui64(hex);
				// Debug(DEBUG_NORM, "setcolbrightnessvalue: hex: '%s', ihex: %" PRIx64 ", bri: '%s', iswhite: '%s'", hex.c_str(), ihex, brightness.c_str(), iswhite.c_str());
				uint8_t r = 0;
				uint8_t g = 0;
				uint8_t b = 0;
				uint8_t cw = 0;
				uint8_t ww = 0;
				switch (hex.length())
				{
					case 6: // RGB
						r = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						g = (uint8_t)((ihex & 0x000000FF00) >> 8);
						b = (uint8_t)ihex & 0xFF;
						float hsb[3];
						int tr, tg, tb; // tmp of 'int' type so can be passed as references to hsb2rgb
						rgb2hsb(r, g, b, hsb);
						// Normalize RGB to full brightness
						hsb2rgb(hsb[0] * 360.0F, hsb[1], 1.0F, tr, tg, tb, 255);
						r = (uint8_t)tr;
						g = (uint8_t)tg;
						b = (uint8_t)tb;
						brightnessAdj = hsb[2];
						// Backwards compatibility: set iswhite for unsaturated colors
						iswhite = (hsb[1] < (20.0 / 255.0)) ? "true" : "false";
						color = _tColor(r, g, b, cw, ww, ColorModeRGB);
						break;
					case 8: // RGB_WW
						r = (uint8_t)((ihex & 0x00FF000000) >> 24);
						g = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						b = (uint8_t)((ihex & 0x000000FF00) >> 8);
						ww = (uint8_t)ihex & 0xFF;
						color = _tColor(r, g, b, cw, ww, ColorModeCustom);
						break;
					case 10: // RGB_CW_WW
						r = (uint8_t)((ihex & 0xFF00000000) >> 32);
						g = (uint8_t)((ihex & 0x00FF000000) >> 24);
						b = (uint8_t)((ihex & 0x0000FF0000) >> 16);
						cw = (uint8_t)((ihex & 0x000000FF00) >> 8);
						ww = (uint8_t)ihex & 0xFF;
						color = _tColor(r, g, b, cw, ww, ColorModeCustom);
						break;
				}
				if (iswhite == "true")
					color.mode = ColorModeWhite;
				// Debug(DEBUG_NORM, "setcolbrightnessvalue: trgbww: %02x%02x%02x%02x%02x, color: '%s'", r, g, b, cw, ww, color.toString().c_str());
			}
			else if (!hue.empty())
			{
				int r, g, b;

				// convert hue to RGB
				float iHue = float(atof(hue.c_str()));
				float iSat = 100.0F;
				if (!sat.empty())
					iSat = float(atof(sat.c_str()));
				hsb2rgb(iHue, iSat / 100.0F, 1.0F, r, g, b, 255);

				color = _tColor((uint8_t)r, (uint8_t)g, (uint8_t)b, 0, 0, ColorModeRGB);
				if (iswhite == "true")
					color.mode = ColorModeWhite;
				// Debug(DEBUG_NORM, "setcolbrightnessvalue2: hue: %f, rgb: %02x%02x%02x, color: '%s'", iHue, r, g, b, color.toString().c_str());
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

			Log(LOG_STATUS, "setcolbrightnessvalue: ID: %" PRIx64 ", bri: %d, color: '%s'", idx, ival, color.toString().c_str());

			// Prevent MQTT update being send to client after next update
			m_LastUpdatedDeviceRowIdx = idx;

			if (!m_mainworker.SwitchLight(idx, "Set Color", ival, color, false, 0, "MQTT") == true)
			{
				Log(LOG_ERROR, "Error sending switch command!");
			}
		}
		else if (szCommand == "switchscene")
		{
			std::string switchcmd = root["switchcmd"].asString();
			if ((switchcmd != "On") && (switchcmd != "Off") && (switchcmd != "Toggle"))
				goto mqttinvaliddata;

			// Prevent MQTT update being send to client after next update
			m_LastUpdatedSceneRowIdx = idx;

			if (!m_mainworker.SwitchScene(idx, switchcmd, "MQTT") == true)
			{
				Log(LOG_ERROR, "Error sending scene command!");
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
				Log(LOG_ERROR, "Error setting uservariable (%s)", errorMessage.c_str());
			}
		}
		else if (szCommand == "addlogmessage")
		{
			std::string msg = root["message"].asString();
			Log(LOG_STATUS, "MQTT MSG: %s", msg.c_str());
		}
		else if (szCommand == "customevent")
		{
			Json::Value eventInfo;
			eventInfo["name"] = root["event"];
			eventInfo["data"] = root["data"];

			if (eventInfo["name"].empty())
			{
				return;
			}

			m_mainworker.m_notificationsystem.Notify(Notification::DZ_CUSTOM, Notification::STATUS_INFO, JSonToRawString(eventInfo));
		}
		else if (szCommand == "sendnotification")
		{
			uint64_t idx = 0;
			std::string name, subject, body, extradata, sound;
			std::string subsystems = NOTIFYALL;
			bool bfromnotification = true;
			int priority = 0;
			if (!root["idx"].empty())
			{
				idx = root["idx"].asUInt64();
			}
			if (!root["name"].empty())
			{
				name = root["name"].asString();
			}
			if (!root["subsystems"].empty())
			{
				subsystems = root["subsystems"].asString();
			}
			if (!root["subject"].empty())
			{
				subject = root["subject"].asString();
			}
			if (!root["body"].empty())
			{
				body = root["body"].asString();
			}
			if (!root["extradata"].empty())
			{
				extradata = root["extradata"].asString();
			}
			if (!root["priority"].empty())
			{
				priority = root["priority"].asInt();
			}
			if (!root["sound"].empty())
			{
				sound = root["sound"].asString();
			}
			if (!root["bfromnotification"].empty())
			{
				bfromnotification = root["bfromnotification"].asBool();
			}
			m_notifications.SendMessageEx(idx, name, subsystems, subject, body, extradata, priority, sound, bfromnotification);
		}
		else if (szCommand == "getdeviceinfo")
		{
			int HardwareID = atoi(result[0][0].c_str());
			SendDeviceInfo(HardwareID, idx, "request device", nullptr);
		}
		else if (szCommand == "getsceneinfo")
		{
			SendSceneInfo(idx, "request scene");
		}
		else
		{
			Log(LOG_ERROR, "Unknown command received: %s", szCommand.c_str());
			return;
		}
		return;
	}
	catch (const Json::LogicError &)
	{
		goto mqttinvaliddata;
	}
mqttinvaliddata:
	Log(LOG_ERROR, "Invalid/Unhandled data received! (Topic: %s, Message: %s)", topic.c_str(), qMessage.c_str());
}

void MQTT::on_disconnect(int rc)
{
	if (rc != 0)
	{
		if (!IsStopRequested(0))
		{
			if (rc == 5)
			{
				Log(LOG_ERROR, "disconnected, Invalid Username/Password (rc=%d)", rc);
			}
			else
			{
				Log(LOG_ERROR, "disconnected, restarting (rc=%d)", rc);
			}
			m_subscribed_topics.clear();
			m_discovered_devices.clear();
			m_discovered_sensors.clear();
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
	Log(LOG_STATUS, "Connecting to %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	int rc;
	int keepalive = 40;

	if (!m_CAFilename.empty())
	{
		rc = tls_opts_set(SSL_VERIFY_NONE, szTLSVersions[m_TLS_Version], nullptr);
		rc = tls_set(m_CAFilename.c_str());
		rc = tls_insecure_set(true);

		if (rc != MOSQ_ERR_SUCCESS)
		{
			Log(LOG_ERROR, "Failed enabling TLS mode, return code: %d (CA certificate: '%s')", rc, m_CAFilename.c_str());
			return false;
		}
		Log(LOG_STATUS, "enabled TLS mode");
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

void MQTT::Do_Work()
{
	bool bFirstTime = true;
	int msec_counter = 0;
	int sec_counter = 0;

	while (!IsStopRequested(100))
	{
		if (!bFirstTime)
		{
			try
			{
				int rc = loop();
				if (rc)
				{
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
			catch (const std::exception &)
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

		msec_counter++;
		if (msec_counter == 10)
		{
			msec_counter = 0;

			sec_counter++;

			if (sec_counter % 12 == 0)
			{
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

	if (m_sDeviceReceivedConnection.connected())
		m_sDeviceReceivedConnection.disconnect();
	if (m_sSwitchSceneConnection.connected())
		m_sSwitchSceneConnection.disconnect();

	Log(LOG_STATUS, "Worker stopped...");
}

void MQTT::SendHeartbeat()
{
	// not necessary for normal MQTT servers
}

void MQTT::SendMessage(const std::string &Topic, const std::string &Message)
{
	if (!m_IsConnected)
	{
		Log(LOG_STATUS, "Not Connected, failed to send message: %s", Message.c_str());
		return;
	}
	if (Topic.empty())
		return;
	try
	{
		publish(nullptr, Topic.c_str(), Message.size(), Message.c_str(), QOS, m_bRetain);
	}
	catch (...)
	{
		Log(LOG_ERROR, "Failed to send message: %s", Message.c_str());
	}
}

void MQTT::WriteInt(const std::string &sendStr)
{
	if (sendStr.size() < 2)
		return;
	// string the return and the end
	std::string sMessage = std::string(sendStr.begin(), sendStr.begin() + sendStr.size());
	SendMessage(m_TopicOut, sMessage);
}

void MQTT::SendDeviceInfo(const int HwdID, const uint64_t DeviceRowIdx, const std::string & /*DeviceName*/, const unsigned char * /*pRXCommand*/)
{
	if (!m_IsConnected)
		return;

	if (m_TopicOut.empty())
		return;

	if (m_bPreventLoop && (DeviceRowIdx == m_LastUpdatedDeviceRowIdx))
	{
		// we should ignore this now
		m_LastUpdatedDeviceRowIdx = 0;
		return;
	}
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT HardwareID, DeviceID, Unit, Name, [Type], SubType, nValue, sValue, SwitchType, SignalLevel, BatteryLevel, Options, Description, LastLevel, Color, LastUpdate "
				  "FROM DeviceStatus WHERE (HardwareID==%d) AND (ID==%" PRIu64 ")",
				  HwdID, DeviceRowIdx);
	if (!result.empty())
	{
		int iIndex = 0;
		std::vector<std::string> sd = result[0];
		std::string hwid = sd[iIndex++];
		std::string did = sd[iIndex++];
		int dunit = atoi(sd[iIndex++].c_str());
		std::string name = sd[iIndex++];
		int dType = atoi(sd[iIndex++].c_str());
		int dSubType = atoi(sd[iIndex++].c_str());
		int nvalue = atoi(sd[iIndex++].c_str());
		std::string svalue = sd[iIndex++];
		_eSwitchType switchType = (_eSwitchType)atoi(sd[iIndex++].c_str());
		int RSSI = atoi(sd[iIndex++].c_str());
		int BatteryLevel = atoi(sd[iIndex++].c_str());
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[iIndex++]);
		std::string description = sd[iIndex++];
		int LastLevel = atoi(sd[iIndex++].c_str());
		std::string sColor = sd[iIndex++];
		std::string sLastUpdate = sd[iIndex++];

		Json::Value root;

		root["idx"] = Json::Value::UInt64(DeviceRowIdx);
		root["hwid"] = hwid;

		if ((dType == pTypeTEMP) || (dType == pTypeTEMP_BARO) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeBARO) || (dType == pTypeHUM) ||
		    (dType == pTypeWIND) || (dType == pTypeRAIN) || (dType == pTypeUV) || (dType == pTypeCURRENT) || (dType == pTypeCURRENTENERGY) || (dType == pTypeENERGY) ||
		    (dType == pTypeRFXMeter) || (dType == pTypeAirQuality) || (dType == pTypeRFXSensor) || (dType == pTypeP1Power) || (dType == pTypeP1Gas))
		{
			try
			{
				root["id"] = std_format("%04X", std::stoi(did));
			}
			catch (const std::exception &)
			{
				//illegal ID here !? probably caused by a plugin/script that does not use numbers as 'ID' (which it should!)
				root["id"] = did;
			}
		}
		else
		{
			root["id"] = did;
		}
		root["unit"] = dunit;
		root["name"] = name;
		root["dtype"] = RFX_Type_Desc((uint8_t)dType, 1);
		root["stype"] = RFX_Type_SubType_Desc((uint8_t)dType, (uint8_t)dSubType);

		if (IsLightOrSwitch(dType, dSubType) == true)
		{
			root["switchType"] = Switch_Type_Desc(switchType);
		}
		else if ((dType == pTypeRFXMeter) || (dType == pTypeRFXSensor))
		{
			root["meterType"] = Meter_Type_Desc((_eMeterType)switchType);
		}
		// Add device options
		for (const auto &option : options)
		{
			std::string optionName = option.first;
			std::string optionValue = option.second;
			root[optionName] = optionValue;
		}

		root["RSSI"] = RSSI;
		root["Battery"] = BatteryLevel;
		root["nvalue"] = nvalue;
		root["description"] = description;
		root["LastUpdate"] = sLastUpdate;

		if (switchType == STYPE_Dimmer)
		{
			root["Level"] = LastLevel;
			if (dType == pTypeColorSwitch)
			{
				_tColor color(sColor);
				root["Color"] = color.toJSONValue();
			}
		}

		// give all svalues separate
		std::vector<std::string> strarray;
		StringSplit(svalue, ";", strarray);

		int sIndex = 1;
		for (const auto &str : strarray)
		{
			std::stringstream szQuery;
			szQuery << "svalue" << sIndex;
			root[szQuery.str()] = str;
			sIndex++;
		}
		std::string message = root.toStyledString();
		if (m_publish_scheme & PT_out)
		{
			SendMessage(m_TopicOut, message);
		}

		if (m_publish_scheme & PT_floor_room)
		{
			result = m_sql.safe_query(
				"SELECT F.Name, P.Name, M.DeviceRowID FROM Plans as P, Floorplans as F, DeviceToPlansMap as M WHERE P.FloorplanID=F.ID and M.PlanID=P.ID and M.DeviceRowID=='%" PRIu64
				"'",
				DeviceRowIdx);
			for (const auto &sd : result)
			{
				std::string floor = sd[0];
				std::string room = sd[1];
				std::stringstream topic;
				topic << m_TopicOut << "/" << floor << "/" + room;

				SendMessage(topic.str(), message);
			}
		}

		if (m_publish_scheme & PT_device_idx)
		{
			std::stringstream topic;
			topic << m_TopicOut << "/" << DeviceRowIdx;
			SendMessage(topic.str(), message);
		}
		if (m_publish_scheme & PT_device_name)
		{
			std::stringstream topic;
			topic << m_TopicOut << "/" << name;
			SendMessage(topic.str(), message);
		}
	}
}

void MQTT::SendSceneInfo(const uint64_t SceneIdx, const std::string & /*SceneName*/)
{
	if (!m_IsConnected)
		return;

	if (m_TopicOut.empty())
		return;

	if (m_bPreventLoop && (SceneIdx == m_LastUpdatedSceneRowIdx))
	{
		// we should ignore this now
		m_LastUpdatedSceneRowIdx = 0;
		return;
	}

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query(
		"SELECT ID, Name, Activators, Favorite, nValue, SceneType, LastUpdate, Protected, OnAction, OffAction, Description FROM Scenes WHERE (ID==%" PRIu64 ") ORDER BY [Order]", SceneIdx);
	if (result.empty())
		return;
	std::vector<std::string> sd = result[0];

	std::string sName = sd[1];
	std::string sLastUpdate = sd[6];

	unsigned char nValue = (uint8_t)atoi(sd[4].c_str());
	unsigned char scenetype = (uint8_t)atoi(sd[5].c_str());
	// int iProtected = atoi(sd[7].c_str());

	// std::string onaction = base64_encode((sd[8]);
	// std::string offaction = base64_encode(sd[9]);

	Json::Value root;

	root["idx"] = atoi(sd[0].c_str());
	root["Name"] = sName;
	// root["Description"] = sd[10];
	// root["Favorite"] = atoi(sd[3].c_str());
	// root["Protected"] = (iProtected != 0);
	// root["OnAction"] = onaction;
	// root["OffAction"] = offaction;

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
	if (m_publish_scheme & PT_out)
	{
		SendMessage(m_TopicOut, message);
	}
}

void MQTT::SubscribeTopic(const std::string &szTopic, const int qos)
{
	if (szTopic.empty())
		return;
	if (m_subscribed_topics.find(szTopic) == m_subscribed_topics.end())
	{
		m_subscribed_topics[szTopic] = true;
		subscribe(nullptr, szTopic.c_str(), qos);
	}
}

void MQTT::CleanValueTemplate(std::string &szValueTemplate)
{
	stdreplace(szValueTemplate, "{", "");
	stdreplace(szValueTemplate, "}", "");
	stdstring_trim(szValueTemplate);
}

std::string MQTT::GetValueTemplateKey(const std::string &szValueTemplate)
{
	std::string szKey;
	std::vector<std::string> strarray;

	if (szValueTemplate.find("value_json.") != std::string::npos)
	{
		StringSplit(szValueTemplate, ".", strarray);
		if (strarray.size() == 2)
		{
			szKey = strarray[1];
		}

	}
	else
	{
		StringSplit(szValueTemplate, ":", strarray);
		if (strarray.size() == 2)
		{
			szKey = strarray[0];
			stdreplace(szKey, "\"", "");
		}
	}
	stdstring_trim(szKey);
	return szKey;
}

void MQTT::on_auto_discovery_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char *)message->payload, (char *)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	std::string component;
	std::string node_id;
	std::string object_id;
	std::string action;
	std::vector<std::string> strarray;
	Json::Value root;

	bool ret = ParseJSon(qMessage, root);
	if ((!ret) || (!root.isObject()))
		goto disovery_invaliddata;

	//topic format: <discovery_prefix>/<component>/[<node_id>/]<object_id>/<action>

	topic = topic.substr(m_TopicDiscoveryPrefix.size() + 1);

	StringSplit(topic, "/", strarray);

	if (strarray.size()== 1)
	{
		//could be offline/online
		return;
	}

	if (!((strarray.size() == 3) || (strarray.size() == 4)))
		goto disovery_invaliddata;

	component = strarray[0]; 

	if (strarray.size() == 3)
	{
		object_id = strarray[1];
		action = strarray[2];
	}
	else
	{
		node_id = strarray[1];
		object_id = strarray[2];
		action = strarray[3];
	}

	//skip some non-needed types, they will be transmitted in the state topic anyways (for 90%)
	if (
		(object_id == "update_available")
		|| (object_id == "power_on_behavior")
		|| (object_id == "power_outage_memory")
		|| (object_id == "update_state")
		|| (object_id == "over-load_status")
		|| (object_id.find("battery") == 0)
		|| (object_id == "linkquality")
		|| (object_id == "sensitivity")
		|| (object_id == "color_temp_startup")
		|| (object_id == "requested_brightness_level")
		|| (object_id == "requested_brightness_percent")
		)
	{
		return;
	}

	if (action != "config")
	{
		Log(LOG_ERROR, "MQTT_Discovery: Something other then 'config' received on discovery topic! (%s/%s)!", topic.c_str(), qMessage.c_str());
		if (action == "state")
		{
			//0/1 (or online/offline)
			return;
		}
		else if (action == "switch")
		{
			// seen with ESP-EASY
			// ON/OFF
			return;
		}

		goto disovery_invaliddata; // could be 'state' with a value 0/1 or switch, but it's not following protocol!
	}
	
	try
	{
		std::string sensor_unique_id;
		if (!root["unique_id"].empty())
			sensor_unique_id = root["unique_id"].asString();
		else if (!root["uniq_id"].empty())
			sensor_unique_id = root["uniq_id"].asString();
		else
		{
			//It's optional, but good to have one
			sensor_unique_id = GenerateUUID();
		}

		if (root["name"].empty())
		{
			//Yes it's optional... but this should be required.. wierd
			root["name"] = sensor_unique_id;
		}

		std::string device_identifiers;

		if (!root["device"].empty())
		{
			if (!root["device"]["identifiers"].empty())
			{
				if (root["device"]["identifiers"].isArray())
					device_identifiers = root["device"]["identifiers"][0].asString();
				else
					device_identifiers = root["device"]["identifiers"].asString();
			}
		}
		else if (!root["dev"].empty())
		{
			if (!root["dev"]["ids"].empty())
			{
				if (root["dev"]["ids"].isArray())
					device_identifiers = root["dev"]["ids"][0].asString();
				else
					device_identifiers = root["dev"]["ids"].asString();
			}
		}
		else
		{
			//It's optional, but good to supply one
		}

		if (device_identifiers.empty())
		{
			// we need a deviceID, let's use the sensors unique id
			device_identifiers = sensor_unique_id;
		}

		//device
		if (m_discovered_devices.find(device_identifiers) == m_discovered_devices.end())
		{
			//new device
			_tMQTTADevice tmpDevice;
			tmpDevice.identifiers = device_identifiers;
			m_discovered_devices[device_identifiers] = tmpDevice;
		}
		_tMQTTADevice *pDevice = &m_discovered_devices[device_identifiers];

		pDevice->identifiers = device_identifiers;
		if (!root["device"]["name"].empty())
			pDevice->name = root["device"]["name"].asString();
		else if (!root["dev"]["name"].empty())
			pDevice->name = root["dev"]["name"].asString();
		if (pDevice->name.empty())
			pDevice->name = pDevice->identifiers;

		if (!root["device"]["sw_version"].empty())
			pDevice->sw_version = root["device"]["sw_version"].asString();
		else if (!root["dev"]["sw"].empty())
			pDevice->sw_version = root["dev"]["sw"].asString();
		if (!root["device"]["model"].empty())
			pDevice->model = root["device"]["model"].asString();
		else if (!root["dev"]["mdl"].empty())
			pDevice->model = root["dev"]["mdl"].asString();
		if (!root["device"]["manufacturer"].empty())
			pDevice->manufacturer = root["device"]["manufacturer"].asString();
		else if (!root["dev"]["mf"].empty())
			pDevice->manufacturer = root["dev"]["mf"].asString();

		for (const auto ittMember : root["device"].getMemberNames())
		{
			if (!root["device"][ittMember].empty())
			{
				if (root["device"][ittMember].isString())
					pDevice->keys[ittMember] = root["device"][ittMember].asString();
			}
		}
		for (const auto ittMember : root["dev"].getMemberNames())
		{
			if (!root["dev"][ittMember].empty())
			{
				if (root["dev"][ittMember].isString())
					pDevice->keys[ittMember] = root["dev"][ittMember].asString();
			}
		}


		//sensor
		if (m_discovered_sensors.find(sensor_unique_id) != m_discovered_sensors.end())
		{
			//Already received
			return;
		}
		_tMQTTASensor tmpSensor;
		m_discovered_sensors[sensor_unique_id] = tmpSensor;
		_tMQTTASensor *pSensor = &m_discovered_sensors[sensor_unique_id];
		pSensor->unique_id = sensor_unique_id;
		pSensor->object_id = object_id;
		pSensor->config = qMessage;
		pSensor->component_type = component;
		pSensor->device_identifiers = device_identifiers;
		pSensor->name = root["name"].asString();

		if (!root["availability_topic"].empty())
			pSensor->availability_topic = root["availability_topic"].asString();
		else if (!root["avty_t"].empty())
			pSensor->availability_topic = root["avty_t"].asString();
		else
		{
			Json::Value rootAvailability;
			if (!root["availability"].empty())
				rootAvailability = root["availability"];
			else if (!root["avty"].empty())
				rootAvailability = root["avty"];
			for (const auto &ittAvail : rootAvailability)
			{
				if (!ittAvail["topic"].empty())
					pSensor->availability_topic = ittAvail["topic"].asString();
				if (!ittAvail["payload_available"].empty())
					pSensor->payload_available = ittAvail["payload_available"].asString();
				if (!ittAvail["payload_not_available"].empty())
					pSensor->payload_not_available = ittAvail["payload_not_available"].asString();
			}
		}
		if (!root["state_topic"].empty())
			pSensor->state_topic = root["state_topic"].asString();
		else if (!root["stat_t"].empty())
			pSensor->state_topic = root["stat_t"].asString();
		else if (!root["json_attributes_topic"].empty())
			pSensor->state_topic = root["json_attributes_topic"].asString();

		if (!root["command_topic"].empty())
			pSensor->command_topic = root["command_topic"].asString();
		else if (!root["cmd_t"].empty())
			pSensor->command_topic = root["cmd_t"].asString();

		if (!root["position_topic"].empty())
			pSensor->position_topic = root["position_topic"].asString();
		if (!root["pos_t"].empty())
			pSensor->position_topic = root["pos_t"].asString();
		if (!root["position_template"].empty())
			pSensor->position_template = root["position_template"].asString();
		if (!root["pos_tpl"].empty())
			pSensor->position_template = root["pos_tpl"].asString();
		CleanValueTemplate(pSensor->position_template);

		if (!root["set_position_topic"].empty())
			pSensor->set_position_topic = root["set_position_topic"].asString();
		else if (!root["set_pos_t"].empty())
			pSensor->set_position_topic = root["set_pos_t"].asString();
		if (!root["set_position_template"].empty())
			pSensor->set_position_template = root["set_position_template"].asString();
		if (!root["set_pos_tpl"].empty())
			pSensor->set_position_template = root["set_pos_tpl"].asString();
		CleanValueTemplate(pSensor->set_position_template);

		if (!root["brightness_command_topic"].empty())
			pSensor->brightness_command_topic = root["brightness_command_topic"].asString();
		else if (!root["bri_cmd_t"].empty())
			pSensor->brightness_command_topic = root["bri_cmd_t"].asString();
		if (!root["brightness_state_topic"].empty())
			pSensor->brightness_state_topic = root["brightness_state_topic"].asString();
		else if (!root["bri_stat_t"].empty())
			pSensor->brightness_state_topic = root["bri_stat_t"].asString();
		if (!root["brightness_scale"].empty())
			pSensor->brightness_scale = root["brightness_scale"].asFloat();
		else if (!root["bri_scl"].empty())
			pSensor->brightness_scale = root["bri_scl"].asFloat();
		if (!root["brightness_value_template"].empty())
			pSensor->brightness_value_template = root["brightness_value_template"].asString();
		else if (!root["bri_val_tpl"].empty())
			pSensor->brightness_value_template = root["bri_val_tpl"].asString();
		CleanValueTemplate(pSensor->brightness_value_template);
		

		if (!root["unit_of_measurement"].empty())
			pSensor->unit_of_measurement = root["unit_of_measurement"].asString();
		else if (!root["unit_of_meas"].empty())
			pSensor->unit_of_measurement = root["unit_of_meas"].asString();

		if (!root["value_template"].empty())
			pSensor->value_template = root["value_template"].asString();
		else if (!root["val_tpl"].empty())
			pSensor->value_template = root["val_tpl"].asString();
		CleanValueTemplate(pSensor->value_template);

		if (!root["state_value_template"].empty())
			pSensor->state_value_template = root["state_value_template"].asString();
		else if (!root["stat_val_tpl"].empty())
			pSensor->state_value_template = root["stat_val_tpl"].asString();

		if (pSensor->value_template.find("value_json") == 0)
			pSensor->value_template = pSensor->value_template.substr(strlen("value_json."));

		if (!root["icon"].empty())
			pSensor->icon = root["icon"].asString();
		else if (!root["ic"].empty())
			pSensor->icon = root["ic"].asString();
		if (!root["payload_on"].empty())
			pSensor->payload_on = root["payload_on"].asString();
		else if (!root["pl_on"].empty())
			pSensor->payload_on = root["pl_on"].asString();
		if (!root["payload_off"].empty())
			pSensor->payload_off = root["payload_off"].asString();
		else if (!root["pl_off"].empty())
			pSensor->payload_off = root["pl_off"].asString();
		if (!root["payload_available"].empty())
			pSensor->payload_available = root["payload_available"].asString();
		else if (!root["pl_avail"].empty())
			pSensor->payload_available = root["pl_avail"].asString();
		if (!root["payload_not_available"].empty())
			pSensor->payload_not_available = root["payload_not_available"].asString();
		else if (!root["pl_not_avail"].empty())
			pSensor->payload_not_available = root["pl_not_avail"].asString();
		if (!root["state_on"].empty())
			pSensor->state_on = root["state_on"].asString();
		else if (!root["stat_on"].empty())
			pSensor->state_on = root["stat_on"].asString();
		if (!root["state_off"].empty())
			pSensor->state_on = root["state_off"].asString();
		else if (!root["stat_off"].empty())
			pSensor->state_on = root["stat_off"].asString();

		if (!root["brightness"].empty())
			pSensor->bBrightness = (root["brightness"].asString() == "true");
		if (!root["color_mode"].empty()) // documentation is a bit unclear, color_mode = true, hs, rgb
			pSensor->bColor_mode = (root["color_mode"].asString() != "false");
		if (!root["supported_color_modes"].empty())
		{
			// onoff, brightness, color_temp, hs, xy, rgb, rgbw, rgbww.
			std::map<std::string, uint8_t> _modes;
			for (const auto &itt : root["supported_color_modes"])
			{
				_modes[itt.asString()] = 1;
			}
			if (_modes.find("rgbww") != _modes.end())
				pSensor->supported_color_modes = "rgbww";
			else if (_modes.find("rgbw") != _modes.end())
				pSensor->supported_color_modes = "rgbw";
			else if (_modes.find("rgb") != _modes.end())
				pSensor->supported_color_modes = "rgb";
			else if (_modes.find("color_temp") != _modes.end())
				pSensor->supported_color_modes = "color_temp";
			else if (_modes.find("xy") != _modes.end())
				pSensor->supported_color_modes = "xy";
			else if (_modes.find("hs") != _modes.end())
				pSensor->supported_color_modes = "hs";
			if (_modes.find("brightness") != _modes.end())
				pSensor->bBrightness = true;
		}
		if (!root["min_mireds"].empty())
			pSensor->min_mireds = root["min_mireds"].asInt();
		if (!root["min_mirs"].empty())
			pSensor->min_mireds = root["min_mirs"].asInt();
		if (!root["max_mireds"].empty())
			pSensor->max_mireds = root["max_mireds"].asInt();
		if (!root["max_mirs"].empty())
			pSensor->max_mireds = root["max_mirs"].asInt();

		
		//Climate
		if (!root["mode_command_topic"].empty())
			pSensor->mode_command_topic = root["mode_command_topic"].asString();
		if (!root["mode_cmd_t"].empty())
			pSensor->mode_command_topic = root["mode_cmd_t"].asString();
		if (!root["mode_state_topic"].empty())
			pSensor->mode_state_topic = root["mode_state_topic"].asString();
		if (!root["mode_stat_t"].empty())
			pSensor->mode_state_topic = root["mode_stat_t"].asString();
		if (!root["mode_state_template"].empty())
			pSensor->mode_state_template = root["mode_state_template"].asString();
		if (!root["mode_stat_tpl"].empty())
			pSensor->mode_state_template = root["mode_stat_tpl"].asString();
		if (!root["modes"].empty())
		{
			for (const auto& ittMode : root["modes"])
			{
				pSensor->climate_modes.push_back(ittMode.asString());
			}
		}

		if (!root["temperature_command_topic"].empty())
			pSensor->temperature_command_topic = root["temperature_command_topic"].asString();
		if (!root["temp_cmd_t"].empty())
			pSensor->temperature_command_topic = root["temp_cmd_t"].asString();
		if (!root["temperature_state_topic"].empty())
			pSensor->temperature_state_topic = root["temperature_state_topic"].asString();
		if (!root["temp_stat_t"].empty())
			pSensor->temperature_state_topic = root["temp_stat_t"].asString();
		if (!root["temperature_state_template"].empty())
			pSensor->temperature_state_template = root["temperature_state_template"].asString();
		if (!root["temp_stat_tpl"].empty())
			pSensor->temperature_state_template = root["temp_stat_tpl"].asString();
		if (!root["temperature_unit"].empty())
			pSensor->temperature_unit = root["temperature_unit"].asString();
		if (!root["temp_unit"].empty())
			pSensor->temperature_unit = root["temp_unit"].asString();
		if (!root["current_temperature_topic"].empty())
			pSensor->current_temperature_topic = root["current_temperature_topic"].asString();
		if (!root["curr_temp_t"].empty())
			pSensor->current_temperature_topic = root["curr_temp_t"].asString();
		if (!root["current_temperature_template"].empty())
			pSensor->current_temperature_template = root["current_temperature_template"].asString();
		if (!root["curr_temp_tpl"].empty())
			pSensor->current_temperature_template = root["curr_temp_tpl"].asString();

		CleanValueTemplate(pSensor->mode_state_template);
		CleanValueTemplate(pSensor->temperature_state_template);
		CleanValueTemplate(pSensor->current_temperature_template);

		if (!root["qos"].empty())
			pSensor->qos = atoi(root["qos"].asString().c_str());

		for (const auto ittMember : root.getMemberNames())
		{
			if (root[ittMember].isString())
				pSensor->keys[ittMember] = root[ittMember].asString();
		}

		pDevice->sensor_ids[pSensor->unique_id] = true;

		Log(LOG_STATUS, "discovered: %s/%s (unique_id: %s)", pDevice->name.c_str(), pSensor->name.c_str(), pSensor->unique_id.c_str());

		//Sanity checks
		if (pSensor->component_type == "sensor")
		{
			if (pSensor->state_topic.empty())
			{
				Log(LOG_ERROR, "A sensor should have a state_topic!");
				return;
			}
		}
		else if (
			(pSensor->component_type == "switch")
			|| (pSensor->component_type == "light")
			)
		{
			if (pSensor->command_topic.empty())
			{
				Log(LOG_ERROR, "Missing command_topic!");
				return;
			}
			InsertUpdateSwitch(pSensor);
		}
		else if (pSensor->component_type == "cover")
		{
			InsertUpdateSwitch(pSensor);
		}
		else if (pSensor->component_type == "climate")
		{
			if (pSensor->temperature_command_topic.empty())
			{
				Log(LOG_ERROR, "Missing temperature_command_topic!");
				return;
			}
		}
		else if (pSensor->component_type == "binary_sensor")
		{
			handle_auto_discovery_binary_sensor(pSensor, message);
		}


		//Check if we want to subscribe to this sensor
		bool bDoSubscribe = false;

		//Only add component_type = "sensor" for now
		bDoSubscribe =
			((pSensor->component_type == "sensor") || (pSensor->component_type == "binary_sensor") || (pSensor->component_type == "switch") || (pSensor->component_type == "light") || (pSensor->component_type == "cover") || (pSensor->component_type == "climate"));

		if (bDoSubscribe)
		{
			SubscribeTopic(pSensor->availability_topic, pSensor->qos);
			SubscribeTopic(pSensor->state_topic, pSensor->qos);
			SubscribeTopic(pSensor->position_topic, pSensor->qos);
			SubscribeTopic(pSensor->brightness_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->mode_state_topic, pSensor->qos);
			SubscribeTopic(pSensor->temperature_state_template, pSensor->qos);
			SubscribeTopic(pSensor->current_temperature_topic, pSensor->qos);
		}
	}
	catch (const std::exception &e)
	{
		Log(LOG_ERROR, "MQTT_Discovery: Error: %s!", e.what());
		goto disovery_invaliddata;
	}
	return;
disovery_invaliddata:
	Log(LOG_ERROR, "MQTT_Discovery: Invalid/Unhandled data received! (Topic: %s, Message: %s)", topic.c_str(), qMessage.c_str());
}

void MQTT::handle_auto_discovery_sensor_message(const struct mosquitto_message *message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char *)message->payload, (char *)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	for (auto& itt : m_discovered_sensors)
	{
		_tMQTTASensor *pSensor = &itt.second;

		if (
			(pSensor->state_topic == topic)
			|| (pSensor->position_topic == topic)
			|| (pSensor->brightness_state_topic == topic)
			|| (pSensor->mode_state_topic == topic)
			|| (pSensor->temperature_state_topic == topic)
			|| (pSensor->current_temperature_topic == topic)
			
			)
		{
			bool bIsJSON = false;
			Json::Value root;
			bool ret = ParseJSon(qMessage, root);
			if (ret)
			{
				bIsJSON = root.isObject();
			}
			std::string szValue;
			if (bIsJSON)
			{
				if (!root["linkquality"].empty())
				{
					pSensor->SignalLevel = (int)round((10.0F / 255.0F) * root["linkquality"].asFloat());
				}
				if (!root["battery"].empty())
				{
					pSensor->BatteryLevel = root["battery"].asInt();
				}
				
				std::string value_template = pSensor->value_template;
				if (!value_template.empty())
				{
					Json::Value key_value = root;

					if (value_template.find('.') != std::string::npos)
					{
						std::vector<std::string> strarray;
						StringSplit(value_template, ".", strarray);
						if (strarray.size() > 1)
						{
							for (auto ittKey : strarray)
							{
								if (!key_value[ittKey].empty())
								{
									if (key_value[ittKey].isObject())
										key_value = key_value[ittKey];
									else
										value_template = ittKey;
								}
								else
								{
									// key not found!
									continue;
								}
							}
						}
					}

					if (key_value[value_template].empty())
						continue; // key not found
					szValue = key_value[value_template].asString();
				}
				else
					szValue = qMessage;
			}
			else
			{
				szValue = qMessage;
			}
			pSensor->last_value = szValue;
			pSensor->last_received = mytime(nullptr);
#ifdef _DEBUG
			std::string szLogMessage = std_format("%s (value: %s", pSensor->name.c_str(), pSensor->last_value.c_str());
			if (!pSensor->unit_of_measurement.empty())
			{
				szLogMessage += " " + utf8_to_string(pSensor->unit_of_measurement);
			}
			szLogMessage += ")";
			Log(LOG_NORM, "MQTT received: %s", szLogMessage.c_str());
#endif
			if (pSensor->component_type == "sensor")
				handle_auto_discovery_sensor(pSensor, message);
			else if (pSensor->component_type == "switch")
				handle_auto_discovery_switch(pSensor, message);
			else if (pSensor->component_type == "binary_sensor")
				handle_auto_discovery_binary_sensor(pSensor, message);
			else if (pSensor->component_type == "light")
				handle_auto_discovery_light(pSensor, message);
			else if (pSensor->component_type == "cover")
				handle_auto_discovery_cover(pSensor, message);
			else if (pSensor->component_type == "climate")
				handle_auto_discovery_climate(pSensor, message);
		}
		else if (pSensor->availability_topic == topic)
		{
			handle_auto_discovery_availability(pSensor, qMessage, message);
		}
	}
}

uint64_t MQTT::UpdateValueInt(int HardwareID, const char *ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
			      const char *sValue, std::string &devname, bool bUseOnOffAction)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(HardwareID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction);
	if (DeviceRowIdx == (uint64_t)-1)
		return -1;
	if (m_bOutputLog)
	{
		std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
		Log(LOG_NORM, szLogString);
	}
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
	m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
	m_mainworker.CheckSceneCode(DeviceRowIdx, devType, subType, nValue, sValue, "MQTT Auto");
	return DeviceRowIdx;
}

void MQTT::handle_auto_discovery_availability(_tMQTTASensor *pSensor, const std::string &payload, const struct mosquitto_message* message)
{
	if (message->retain)
		return;
	if (pSensor->devType == 0)
		return; //not in system yet
	pSensor->bOnline = (payload == "online");
	if (!pSensor->payload_available.empty())
		pSensor->bOnline = (payload == pSensor->payload_available);
	else if (!pSensor->payload_not_available.empty())
		pSensor->bOnline = !(payload == pSensor->payload_not_available);
	if (pSensor->bOnline)
	{
		// update last seen
		m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", TimeToString(nullptr, TF_DateTime).c_str(), m_HwdID,
				 pSensor->unique_id.c_str(), pSensor->devUnit);
	}
}

MQTT::_tMQTTASensor* MQTT::get_auto_discovery_sensor_unit(_tMQTTASensor* pSensor, const std::string& szMeasurementUnit)
{
	//Retrieved sensor from same device with specified measurement unit
	_tMQTTADevice* pDevice = &m_discovered_devices[pSensor->device_identifiers];
	if (pDevice == nullptr)
		return nullptr; //device not found!?

	for (const auto ittSensorID : pDevice->sensor_ids)
	{
		if (m_discovered_sensors.find(ittSensorID.first) != m_discovered_sensors.end())
		{
			_tMQTTASensor* pDeviceSensor = &m_discovered_sensors[ittSensorID.first];
			if (pDeviceSensor->unit_of_measurement == szMeasurementUnit)
				return pDeviceSensor;
		}
	}
	return nullptr;
}

MQTT::_tMQTTASensor* MQTT::get_auto_discovery_sensor_unit(_tMQTTASensor* pSensor, const uint8_t devType, const int subType, const int devUnit)
{
	//Retrieved sensor from same device with specified device type/subtype/unit
	_tMQTTADevice* pDevice = &m_discovered_devices[pSensor->device_identifiers];
	if (pDevice == nullptr)
		return nullptr; //device not found!?

	for (const auto ittSensorID : pDevice->sensor_ids)
	{
		if (m_discovered_sensors.find(ittSensorID.first) != m_discovered_sensors.end())
		{
			_tMQTTASensor* pDeviceSensor = &m_discovered_sensors[ittSensorID.first];
			if (pDeviceSensor->devType == devType)
			{
				if ((pDeviceSensor->subType == subType) || (subType == -1))
				{
					if ((pDeviceSensor->devUnit == devUnit) || (devUnit == -1))
					{
						return pDeviceSensor;
					}
				}
			}
		}
	}
	return nullptr;

}

void MQTT::handle_auto_discovery_sensor(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	pSensor->devUnit = 1;
	int devType = pTypeGeneral;
	int subType = sTypeCustom;
	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	bool bIsTemp = false;
	bool bIsHum = false;
	bool bIsBaro = false;

	std::string szUnit = utf8_to_string(pSensor->unit_of_measurement);

	if (szUnit.empty())
	{
		char* p;
		double converted = strtod(pSensor->last_value.c_str(), &p);
		if (*p)
		{
			// conversion failed because the input wasn't a number
			// make it a text sensor
			szUnit = "Text";
		}
	}

	if (szUnit == "dBm")
	{
		// probably WIFI strength, should be handled differently and used as signal strength
		return;
	}
	else if (
		(szUnit == "°C")
		|| (szUnit == "C")
		|| (pSensor->value_template.find("temperature") != std::string::npos)
		|| (pSensor->state_topic.find("temperature") != std::string::npos)
		)
	{
		bIsTemp = true;
		devType = pTypeTEMP;
		subType = sTypeTEMP1;

		float temp = static_cast<float>(atof(pSensor->last_value.c_str()));
		m_sql.GetAddjustment(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
		pSensor->sValue = std_format("%.1f", temp);
	}
	else if (szUnit == "%")
	{
		if (
			(pSensor->object_id.find("humidity") != std::string::npos)
			|| (pSensor->state_topic.find("humidity") != std::string::npos)
			|| (pSensor->unique_id.find("humidity") != std::string::npos)
			|| (pSensor->value_template.find("humidity") != std::string::npos)
			)
		{
			bIsHum = true;
			devType = pTypeHUM;
			subType = sTypeHUM2;
			pSensor->nValue = atoi(pSensor->last_value.c_str());
			pSensor->sValue = std_format("%d", Get_Humidity_Level(pSensor->nValue));
		}
		else
		{
			devType = pTypeGeneral;
			subType = sTypeCustom;
			pSensor->szOptions = pSensor->unit_of_measurement;
			pSensor->sValue = pSensor->last_value;
		}
	}
	else if (
		(szUnit == "hPa")
		|| (szUnit == "kPa")
		|| (pSensor->value_template.find("pressure") != std::string::npos)
		)
	{
		bIsBaro = true;
		devType = pTypeGeneral;
		subType = sTypeBaro;
		float pressure = static_cast<float>(atof(pSensor->last_value.c_str()));
		if (szUnit == "kPa")
			pressure *= 10.0F;
		int nforecast = bmpbaroforecast_cloudy;
		if (pressure <= 980)
			nforecast = bmpbaroforecast_thunderstorm;
		else if (pressure <= 995)
			nforecast = bmpbaroforecast_rain;
		else if (pressure >= 1029)
			nforecast = bmpbaroforecast_sunny;
		pSensor->sValue = std_format("%.02f;%d", pressure, nforecast);
	}
	else if (szUnit == "ppm")
	{
		devType = pTypeAirQuality;
		subType = sTypeVoltcraft;
		pSensor->nValue = atoi(pSensor->last_value.c_str());
	}
	else if (szUnit == "µg/m³")
	{
		devType = pTypeGeneral;
		subType = sTypeCustom;
		pSensor->szOptions = pSensor->unit_of_measurement;
		pSensor->sValue = pSensor->last_value;
	}
	else if (szUnit == "V")
	{
		devType = pTypeGeneral;
		subType = sTypeVoltage;
		pSensor->sValue = pSensor->last_value;
	}
	else if (szUnit == "mV")
	{
		devType = pTypeGeneral;
		subType = sTypeVoltage;
		pSensor->sValue = std_format("%.3f", static_cast<float>(atof(pSensor->last_value.c_str()))/1000.0F);
	}
	else if (szUnit == "A")
	{
		devType = pTypeGeneral;
		subType = sTypeCurrent;
		pSensor->sValue = pSensor->last_value;
	}
	else if (szUnit == "W")
	{
		devType = pTypeUsage;
		subType = sTypeElectric;
		pSensor->sValue = pSensor->last_value;
	}
	else if (szUnit == "kWh")
	{
		devType = pTypeGeneral;
		subType = sTypeKwh;

		float fUsage = 0;
		float fkWh = static_cast<float>(atof(pSensor->last_value.c_str()));
		_tMQTTASensor* pWattSensor = get_auto_discovery_sensor_unit(pSensor, "W");
		if (pWattSensor)
		{
			fUsage = static_cast<float>(atof(pSensor->last_value.c_str()));
		}
		pSensor->sValue = std_format("%.3f;%.3f", fUsage, fkWh);
	}
	else if (
		(szUnit == "lx")
		||(szUnit == "lux")
		|| (pSensor->state_topic.find("illuminance_lux") != std::string::npos)
		)
	{
		devType = pTypeLux;
		subType = sTypeLux;
		pSensor->sValue = std_format("%.0f", static_cast<float>(atof(pSensor->last_value.c_str())));
	}
	else if (szUnit == "Text")
	{
		devType = pTypeGeneral;
		subType = sTypeTextStatus;
		pSensor->sValue = pSensor->last_value;
	}
	else
	{
		devType = pTypeGeneral;
		subType = sTypeCustom;
		pSensor->szOptions = pSensor->unit_of_measurement;
		pSensor->sValue = pSensor->last_value;
	}

	if ((devType == pTypeGeneral) && (subType == sTypeCustom) && pSensor->szOptions.empty())
		pSensor->szOptions = "??";

	pSensor->devType = devType;
	pSensor->subType = subType;

	if (bIsTemp || bIsHum || bIsBaro)
	{
		if (m_discovered_devices.find(pSensor->device_identifiers) == m_discovered_devices.end())
			return; //not discovered yet!?

		std::string szDeviceID = pSensor->device_identifiers;
		std::string sDeviceName = m_discovered_devices[pSensor->device_identifiers].name;
		int nValue = 0;
		std::string sValue;

		float temp = 0;
		int humidity = 0;
		float pressure = 0;

		_tMQTTASensor* pTempSensor = (bIsTemp) ? pSensor : nullptr;
		_tMQTTASensor* pHumSensor = (bIsHum) ? pSensor : nullptr;
		_tMQTTASensor* pBaroSensor = (bIsBaro) ? pSensor : nullptr;

		if (!pTempSensor)
			pTempSensor = get_auto_discovery_sensor_unit(pSensor, pTypeTEMP, sTypeTEMP1);
		if (!pHumSensor)
			pHumSensor = get_auto_discovery_sensor_unit(pSensor, pTypeHUM, sTypeHUM2);
		if (!pBaroSensor)
			pBaroSensor = get_auto_discovery_sensor_unit(pSensor, pTypeGeneral, sTypeBaro);

		if (pTempSensor)
			temp = static_cast<float>(atof(pTempSensor->last_value.c_str()));
		if (pHumSensor)
			humidity = pHumSensor->nValue;
		if (pBaroSensor)
		{
			pressure = static_cast<float>(atof(pBaroSensor->last_value.c_str()));
			if (pBaroSensor->unit_of_measurement == "kPa")
				pressure *= 10.0F;
		}

		if (pTempSensor && pHumSensor && pBaroSensor)
		{
			//Temp + Hum + Baro
			if (
				(pTempSensor->last_received == 0)
				|| (pHumSensor->last_received == 0)
				|| (pBaroSensor->last_received == 0)
				)
				return; //not all 3 received yet
			//make a new sensor and use the 'device_identifier' as ID
			devType = pTypeTEMP_HUM_BARO;
			subType = sTypeTHBFloat;
			uint8_t nforecast = wsbaroforecast_some_clouds;
			if (pressure <= 980)
				nforecast = wsbaroforecast_heavy_rain;
			else if (pressure <= 995)
			{
				if (temp > 1)
					nforecast = wsbaroforecast_rain;
				else
					nforecast = wsbaroforecast_snow;
			}
			else if (pressure >= 1029)
				nforecast = wsbaroforecast_sunny;
			sValue = std_format("%.1f;%d;%d;%.1f;%d", temp, humidity, Get_Humidity_Level(humidity), pressure, nforecast);
		}
		else if (pTempSensor && pHumSensor)
		{
			//Temp + Hum
			if (
				(pTempSensor->last_received == 0) 
				|| (pHumSensor->last_received == 0)
				)
				return; // not all 2 received yet
			devType = pTypeTEMP_HUM;
			subType = sTypeTH1;
			sValue = std_format("%.1f;%d;%d", temp, humidity, Get_Humidity_Level(humidity));
		}
		else
		{
			devType = pSensor->devType;
			subType = pSensor->subType;
			nValue = pSensor->nValue;
			sValue = pSensor->sValue;
		}
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
			m_HwdID, szDeviceID.c_str(), 1, devType, subType);
		if (result.empty())
		{
			// Insert
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
				"VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
				m_HwdID, szDeviceID.c_str(), devType, subType, pSensor->SignalLevel, pSensor->BatteryLevel, sDeviceName.c_str(), 1, nValue, sValue.c_str());
		}
		else
		{
			// Update
			if (message->retain)
				return; // only update when a new value is received
			UpdateValueInt(m_HwdID, szDeviceID.c_str(), 1, devType, subType, pSensor->SignalLevel, pSensor->BatteryLevel, nValue, sValue.c_str(), result[0][0]);
		}
	}
	else
	{
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
			pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType);
		if (result.empty())
		{
			//Insert
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, Options, nValue, sValue) "
				"VALUES (%d, '%q', %d, %d, %d, %d, %d, '%q', %d, '1;%q', %d, '%q')",
				m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), 1,
				pSensor->szOptions.c_str(), pSensor->nValue, pSensor->sValue.c_str());
		}
		else
		{
			//Update
			if (message->retain)
				return; //only update when a new value is received
			UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
				pSensor->sValue.c_str(), result[0][0]);
		}
	}
}

void MQTT::InsertUpdateSwitch(_tMQTTASensor* pSensor)
{
	pSensor->devUnit = 1;
	pSensor->devType = pTypeGeneralSwitch;
	pSensor->subType = sSwitchGeneralSwitch;
	int Used = 1;

	int switchType = STYPE_OnOff;

	if (pSensor->bColor_mode)
	{
		pSensor->devType = pTypeColorSwitch;
		pSensor->subType = sTypeColor_RGB;

		if (pSensor->supported_color_modes == "rgb")
			pSensor->subType = sTypeColor_RGB;
		else if (pSensor->supported_color_modes == "rgbw")
			pSensor->subType = sTypeColor_RGB_W_Z;
		else if (pSensor->supported_color_modes == "rgbww")
			pSensor->subType = sTypeColor_RGB_CW_WW_Z;
		else if (pSensor->supported_color_modes == "color_temp")
		{
			//Actually I think this is just a normal white bulb
			pSensor->subType = sTypeColor_CW_WW;
		}
		else if (pSensor->supported_color_modes == "brightness")
		{
			//a normal dimmer
			pSensor->devType = pTypeGeneralSwitch;
			pSensor->subType = sSwitchGeneralSwitch;
		}
		else
		{
			Log(LOG_ERROR, "Unhandled color switch type '%s' (%s)", pSensor->supported_color_modes.c_str(), pSensor->name.c_str());
			return;
		}
		switchType = STYPE_Dimmer;
	}
	else if (
		(pSensor->bBrightness)
		||(!pSensor->brightness_command_topic.empty())
		||(!pSensor->brightness_state_topic.empty())
		)
	{
		switchType = STYPE_Dimmer;
	}
	else if (
		(!pSensor->position_topic.empty())
		|| (!pSensor->set_position_topic.empty())
		)
	{
		switchType = STYPE_BlindsPercentage;
	}
	else if (pSensor->component_type == "binary_sensor")
	{
		if ((pSensor->object_id.find("_state") != std::string::npos) || (pSensor->object_id.find("_status") != std::string::npos) ||
		    (pSensor->object_id.find("_unknown") != std::string::npos) || (pSensor->object_id.find("tamper") != std::string::npos))
		{
			//Don't add these into our system used
			Used = 0;
		}
	}

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Color,SubType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, pSensor->unique_id.c_str());
	if (result.empty())
	{
		// New switch, add it to the system
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
				 "VALUES (%d, '%q', 1, %d, %d, %d, %d, %d, '%q', %d, %d, '%q')",
				 m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), Used, 0, "0");
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Color,SubType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, pSensor->unique_id.c_str());
	}
	if (result.empty())
		return; // should not happen!

	if (pSensor->last_value.empty())
		return;

	std::string szIdx = result[0][0];
	uint64_t DevRowIdx = std::stoull(szIdx);
	std::string szDeviceName = result[0][1];
	int nValue = atoi(result[0][2].c_str());
	std::string sValue = result[0][3];
	std::string sColor = result[0][4];
	int subType = atoi(result[0][5].c_str());

	if (pSensor->subType != subType)
		m_sql.UpdateDeviceValue("SubType", subType, szIdx);
	pSensor->subType = subType;

	bool bOn = false;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(pSensor->last_value, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}
	std::string szOnOffValue = pSensor->last_value;
	int level = 0;

	_tColor color_old;
	_tColor color_new;
	color_old.mode = ColorModeCustom;
	color_new.mode = ColorModeCustom;
	bool bHaveColorChange = false;

	if (bIsJSON)
	{
		if (!root["state"].empty())
			szOnOffValue = root["state"].asString();
		else if (!root["value"].empty())
		{
			int iValue = root["value"].asInt();
			szOnOffValue = (iValue != 0) ? pSensor->payload_on : pSensor->payload_off;
		}
		else
		{
			Log(LOG_ERROR, "Unhandled state received '%s' (%s/%s)", pSensor->last_value.c_str(), pSensor->unique_id.c_str(), szDeviceName.c_str());
			return;
		}
		if (!root["brightness"].empty())
		{
			double dLevel = (100.0 / 255.0) * root["brightness"].asInt();
			level = (int)dLevel;
		}
		if (!root["position"].empty())
		{
			level = root["position"].asInt();
		}
		if (!root["color"].empty())
		{
			Json::Value root_color;
			bool ret = ParseJSon(sColor, root_color);
			if (ret)
			{
				if (root_color.isObject())
				{
					color_old.fromJSON(root_color);
				}
			}

			if (!root["color"]["r"].empty())
				color_new.r = root["color"]["r"].asInt();
			if (!root["color"]["g"].empty())
				color_new.g = root["color"]["g"].asInt();
			if (!root["color"]["b"].empty())
				color_new.b = root["color"]["b"].asInt();
			if (!root["color"]["c"].empty())
				color_new.cw = root["color"]["c"].asInt();
			if (!root["color"]["w"].empty())
				color_new.ww = root["color"]["w"].asInt();

			if ((!root["color"]["x"].empty()) && (!root["color"]["y"].empty()))
			{
				// convert xy to rgb
				_tColor::RgbFromXY(root["color"]["x"].asDouble(), root["color"]["y"].asDouble(), color_new.r, color_new.g, color_new.b);
			}
			if ((!root["color"]["h"].empty()) && (!root["color"]["s"].empty()))
			{
				// convert hue/sat to rgb
				float iHue = float(root["color"]["h"].asDouble()) * 360.0F / 65535.0F;
				float iSat = float(root["color"]["s"].asDouble()) / 254.0F;
				int r = 0;
				int g = 0;
				int b = 0;
				hsb2rgb(iHue, iSat, 1.0F, r, g, b, 255);
				color_new.r = (uint8_t)r;
				color_new.g = (uint8_t)g;
				color_new.b = (uint8_t)b;
			}
			std::string szColorOld = color_old.toJSONString();
			std::string szColorNew = color_new.toJSONString();
			bHaveColorChange = szColorOld != szColorNew;
		}
		else if (!root["color_temp"].empty())
		{
			float CT = root["color_temp"].asFloat();
			float iCt =( (float(CT) - pSensor->min_mireds) / (pSensor->max_mireds - pSensor->min_mireds)) * 255.0F;
			color_new = _tColor((uint8_t)round(iCt), ColorModeTemp);
			std::string szColorOld = color_old.toJSONString();
			std::string szColorNew = color_new.toJSONString();
			bHaveColorChange = szColorOld != szColorNew;
		}
	}
	else {
		if (is_number(szOnOffValue))
		{
			//must be a level
			level = atoi(szOnOffValue.c_str());
			if (level > 0)
			{
				if (level != 100)
					szOnOffValue = "Set Level";
				else
					szOnOffValue = "on";
			}
			else
				szOnOffValue = "off";
		}
	}

	if (level > 100)
		level = 100;

	int slevel = atoi(sValue.c_str());
	bool bHaveLevelChange = (slevel != level);


	if (szOnOffValue == pSensor->payload_on)
		bOn = true;
	else if (szOnOffValue == pSensor->payload_off)
		bOn = false;
	else
		bOn = (szOnOffValue == "on") || (szOnOffValue == "ON") || (szOnOffValue == "true") || (szOnOffValue == "Set Level");

	// check if we have a change, if not do not update it
	if ((!bOn) && (nValue == 0))
	{
		if (!bHaveLevelChange)
		{
			if (!bHaveColorChange)
				return;
		}
	}
	if ((bOn && (nValue != 0)))
	{
		// Check Level
		int slevel = atoi(sValue.c_str());
		if (slevel == level)
		{
			if (!bHaveColorChange)
				return;
		}
	}
	if (szOnOffValue == "Set Level")
	{
		nValue = gswitch_sSetLevel;
	}
	else
		nValue = (bOn) ? gswitch_sOn : gswitch_sOff;

	sValue = std_format("%d", level);

	pSensor->nValue = nValue;
	pSensor->sValue = sValue;

	UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue, pSensor->sValue.c_str(),
		szDeviceName);
	if (bHaveColorChange)
		m_sql.UpdateDeviceValue("Color", color_new.toJSONString(), szIdx);
}

void MQTT::handle_auto_discovery_switch(_tMQTTASensor *pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTT::handle_auto_discovery_binary_sensor(_tMQTTASensor *pSensor, const struct mosquitto_message* message)
{
	if ((pSensor->object_id.find("any") != std::string::npos) || (pSensor->object_id == "battery_islow") || (pSensor->object_id == "battery_ishigh") ||
	    (pSensor->object_id == "over-load_status") || (pSensor->object_id == "hardware_status") || (pSensor->object_id == "update_available"))
		return; // don't want these

	InsertUpdateSwitch(pSensor);
}

void MQTT::handle_auto_discovery_light(_tMQTTASensor *pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTT::handle_auto_discovery_cover(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	InsertUpdateSwitch(pSensor);
}

void MQTT::handle_auto_discovery_climate(_tMQTTASensor* pSensor, const struct mosquitto_message* message)
{
	std::string topic = message->topic;
	std::string qMessage = std::string((char*)message->payload, (char*)message->payload + message->payloadlen);

	if (qMessage.empty())
		return;

	bool bIsJSON = false;
	Json::Value root;
	bool ret = ParseJSon(qMessage, root);
	if (ret)
	{
		bIsJSON = root.isObject();
	}

	if (
		(pSensor->mode_state_topic == topic)
		&& (!pSensor->climate_modes.empty())
		)
	{
		std::string current_mode;
		if (!bIsJSON)
		{
			Log(LOG_ERROR, "Climate device no idea how to interpretate state values (%s)", pSensor->unique_id.c_str());
			return;
		}
		if (!pSensor->mode_state_template.empty())
		{
			std::string szKey = GetValueTemplateKey(pSensor->mode_state_template);
			if (!szKey.empty())
				current_mode = root[szKey].asString();
			else
			{
				Log(LOG_ERROR, "Climate device no idea how to interpretate state values (%s)", pSensor->unique_id.c_str());
				return;
			}
		}
		else
			current_mode = qMessage;

		pSensor->devType = pTypeGeneralSwitch;
		pSensor->subType = sSwitchGeneralSwitch;
		int switchType = STYPE_Selector;

		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType);
		if (result.empty())
		{
			// New switch, add it to the system
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
					 "VALUES (%d, '%q', 1, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
					 m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, switchType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), 1, 0);
			result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, pSensor->unique_id.c_str(), pSensor->devType,
						  pSensor->subType);
		}
		if (result.empty())
			return; // should not happen!

		if (pSensor->last_value.empty())
			return;

		std::string szIdx = result[0][0];
		uint64_t DevRowIdx = std::stoull(szIdx);
		std::string szDeviceName = result[0][1];
		int nValue = atoi(result[0][2].c_str());
		std::string sValue = result[0][3];
		std::string sOptions = result[0][4];

		int iActualIndex = 0;

		// Build switch options
		int iValueIndex = 0;
		std::string tmpOptionString;
		for (const auto &ittOptions : pSensor->climate_modes)
		{
			if (ittOptions == current_mode)
				iActualIndex = iValueIndex;
			if (!tmpOptionString.empty())
				tmpOptionString += "|";
			tmpOptionString += ittOptions;
			iValueIndex += 10;
		}
		std::map<std::string, std::string> optionsMap;
		optionsMap["SelectorStyle"] = "0";
		optionsMap["LevelOffHidden"] = "false";
		optionsMap["LevelNames"] = tmpOptionString;

		std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
		if (newOptions != sOptions)
			m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

		pSensor->nValue = (iActualIndex == 0) ? 0 : 2;
		pSensor->sValue = std_format("%d", iActualIndex);

		if ((pSensor->nValue != nValue) || (pSensor->sValue != sValue))
		{
			UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
				       pSensor->sValue.c_str(), szDeviceName);
		}
	}
	if (pSensor->temperature_state_topic == topic)
	{
		float temp_setpoint = 0;
		if (bIsJSON)
		{
			//Current Setpoint
			if (!pSensor->temperature_state_template.empty())
			{
				std::string szKey = GetValueTemplateKey(pSensor->temperature_state_template);
				if (!szKey.empty())
					temp_setpoint = root[szKey].asFloat();
				else
				{
					Log(LOG_ERROR, "Climate device unhandled temperature_state_template (%s)", pSensor->unique_id.c_str());
					return;
				}
			}
		}
		else
			temp_setpoint = static_cast<float>(atof(qMessage.c_str()));

		pSensor->nValue = 0;
		pSensor->sValue = std_format("%.2f", temp_setpoint);
		pSensor->devType = pTypeThermostat;
		pSensor->subType = sTypeThermSetpoint;
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
					  pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType);
		if (result.empty())
		{
			// Insert
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
					 "VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
					 m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), 1,
					 pSensor->nValue, pSensor->sValue.c_str());
		}
		else
		{
			// Update
			UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
				       pSensor->sValue.c_str(),
				       result[0][0]);
		}

	}
	if (pSensor->current_temperature_topic == topic)
	{
		float temp_current = 0;
		if (bIsJSON)
		{
			//Current temperature
			if (!pSensor->current_temperature_template.empty())
			{
				std::string szKey = GetValueTemplateKey(pSensor->current_temperature_template);
				if (!szKey.empty())
					temp_current = root[szKey].asFloat();
				else
				{
					Log(LOG_ERROR, "Climate device unhandled current_temperature_template (%s)", pSensor->unique_id.c_str());
					return;
				}
			}
		}
		else
			temp_current = static_cast<float>(atof(qMessage.c_str()));

		pSensor->devType = pTypeTEMP;
		pSensor->subType = sTypeTEMP1;

		pSensor->nValue = 0;

		float AddjValue = 0.0F;
		float AddjMulti = 1.0F;

		m_sql.GetAddjustment(m_HwdID, pSensor->unique_id.c_str(), pSensor->devUnit, pSensor->devType, pSensor->subType, AddjValue, AddjMulti);
		temp_current += AddjValue;
		pSensor->sValue = std_format("%.1f", temp_current);

		pSensor->subType = sTypeThermSetpoint;
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID,
					  pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType);
		if (result.empty())
		{
			// Insert
			m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue) "
					 "VALUES (%d, '%q', 1, %d, %d, %d, %d, '%q', %d, %d, '%q')",
					 m_HwdID, pSensor->unique_id.c_str(), pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->name.c_str(), 1,
					 pSensor->nValue, pSensor->sValue.c_str());
		}
		else
		{
			// Update
			UpdateValueInt(m_HwdID, pSensor->unique_id.c_str(), 1, pSensor->devType, pSensor->subType, pSensor->SignalLevel, pSensor->BatteryLevel, pSensor->nValue,
				       pSensor->sValue.c_str(), result[0][0]);
		}
	}
}

bool MQTT::SendSwitchCommand(const std::string &DeviceID, const std::string &DeviceName, int Unit, std::string command, int level, _tColor color)
{
	if (m_discovered_sensors.find(DeviceID) == m_discovered_sensors.end())
	{
		Log(LOG_ERROR, "Switch not found!? (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
		return false;
	}
	_tMQTTASensor *pSensor = &m_discovered_sensors[DeviceID];

	if (
		(pSensor->component_type != "switch")
		&& (pSensor->component_type != "light")
		&& (pSensor->component_type != "cover")
		&& (pSensor->component_type != "climate")
		)
	{
		Log(LOG_ERROR, "sending switch commands for switch type '%s' is not supported (yet...) (%s/%s)", pSensor->component_type.c_str(), DeviceID.c_str(), DeviceName.c_str());
		return false;
	}

	std::string szSendValue;

	if (pSensor->component_type != "climate")
	{
		if (command == "On")
			szSendValue = pSensor->payload_on;
		else if (command == "Off")
			szSendValue = pSensor->payload_off;
		else if (command == "Set Level")
		{
			if (level == 0)
			{
				command = "Off";
				szSendValue = pSensor->payload_off;
			}
		}
		else if ((command == "Set Color") && (pSensor->component_type == "light"))
		{
			// That's valid
		}
		else
		{
			Log(LOG_ERROR, "Switch command not supported (%s - %s/%s)", command.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}
	}
	
	if (pSensor->component_type == "light")
	{
		Json::Value root;

		if (
			(command == "On")
			|| (command == "Off"))
		{
			if (pSensor->state_value_template.empty())
			{
				if (szSendValue == "true")
					root["state"] = true;
				else if (szSendValue == "false")
					root["state"] = false;
				else
					root["state"] = szSendValue;
			}
			else
			{
				root["value"] = (command == "On") ? 1 : 0;
			}
		}
		else if (command == "Set Level")
		{
			root["state"] = pSensor->payload_on;
			int slevel = (int)((pSensor->brightness_scale / 100.0F) * level);

			if (!pSensor->brightness_value_template.empty())
			{
				std::string szKey = GetValueTemplateKey(pSensor->brightness_value_template);
				if (!szKey.empty())
					root[szKey] = slevel;
				else
				{
					Log(LOG_ERROR, "Cover device unhandled brightness_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
			}
			else
				root["brightness"] = slevel;

			szSendValue = JSonToRawString(root);
			std::string szTopic = pSensor->command_topic;
			if (!pSensor->brightness_command_topic.empty())
				szTopic = pSensor->brightness_command_topic;
			else if (!pSensor->set_position_topic.empty())
				szTopic = pSensor->set_position_topic;
			SendMessage(szTopic, szSendValue);
			return true;
		}
		else if (command == "Set Color")
		{
			if (pSensor->supported_color_modes == "xy")
			{
				double x, y, z;
				_tColor::XYFromRGB(color.r, color.g, color.g, x, y, z);
				root["color"]["x"] = x;
				root["color"]["y"] = y;
			}
			else if (pSensor->supported_color_modes == "color_temp") //seen as XY
			{
				//color.cw color.ww t
				float iCt = pSensor->min_mireds + ((static_cast<float>(pSensor->max_mireds - pSensor->min_mireds) / 255.0F) * color.t);
				root["color_temp"] = (int)round(iCt);

			}
			else if (pSensor->supported_color_modes == "hs")
			{
				float hsb[3];
				rgb2hsb(color.r, color.g, color.b, hsb);
				root["color"]["h"] = hsb[0];
				root["color"]["s"] = hsb[1];
				root["brightness"] = hsb[1] * 255.0F;
			}
			else
			{
				root["state"] = pSensor->payload_on;
				root["color"]["r"] = color.r;
				root["color"]["g"] = color.g;
				root["color"]["b"] = color.b;

				if (pSensor->supported_color_modes == "rgbw")
					root["color"]["c"] = color.cw;
				else if (pSensor->supported_color_modes == "rgbww")
					root["color"]["w"] = color.ww;
			}
			int slevel = (int)((pSensor->brightness_scale / 100.0F) * level);

			if (!pSensor->brightness_value_template.empty())
			{
				std::string szKey = GetValueTemplateKey(pSensor->brightness_value_template);
				if (!szKey.empty())
					root[szKey] = slevel;
				else
				{
					Log(LOG_ERROR, "Cover device unhandled brightness_value_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
			}
			else
				root["brightness"] = slevel;

		}
		else
		{
			Log(LOG_ERROR, "Switch command not supported (%s - %s/%s)", command.c_str(), DeviceID.c_str(), DeviceName.c_str());
			return false;
		}

		szSendValue = JSonToRawString(root);
	}
	else if (pSensor->component_type == "cover")
	{
		Json::Value root;

		if (command == "Set Level")
		{
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Color,SubType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')", m_HwdID, pSensor->unique_id.c_str());
			if (!result.empty())
			{
				m_sql.safe_query(
					"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %s)",
					level, result[0][0].c_str());
			}
			if (!pSensor->set_position_topic.empty())
			{
				if (pSensor->set_position_template.empty())
				{
					szSendValue = std::to_string(level);
				}
				else
				{
					std::string szKey = GetValueTemplateKey(pSensor->set_position_template);
					if (!szKey.empty())
					{
						root[szKey] = level;
						szSendValue = JSonToRawString(root);
					}
					else
					{
						Log(LOG_ERROR, "Cover device unhandled set_position_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
						return false;
					}
				}
				SendMessage(pSensor->set_position_topic, szSendValue);

			}
			else {
				Log(LOG_ERROR, "Cover device does not have set_position_topic (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
				return false;
			}
		}
		else if (command == "On")
		{
			szSendValue = "CLOSE";
		}
		else if (command == "Off")
		{
			szSendValue = "OPEN";
		}
		else if (command == "Stop")
		{
			szSendValue = "STOP";
		}
		SendMessage(pSensor->command_topic, szSendValue);
		return true;
	}
	if (pSensor->component_type == "climate")
	{
		if (command == "Set Level")
		{
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type=%d) AND (SubType=%d)", m_HwdID, pSensor->unique_id.c_str(),
						  pTypeGeneralSwitch, sSwitchGeneralSwitch);
			if (result.empty())
				return false;

			std::string szIdx = result[0][0];

			Json::Value root;
			if (!pSensor->mode_command_topic.empty())
			{
				std::map<std::string, std::string> deviceOptions = m_sql.GetDeviceOptions(szIdx);

				if (deviceOptions.find("LevelNames") == deviceOptions.end())
					return false;
				int iLevel = level / 10;

				std::vector<std::string> strarray;
				StringSplit(deviceOptions["LevelNames"], "|", strarray);
				if (iLevel >= (int)strarray.size())
				{
					Log(LOG_ERROR, "Climate device invalid state requested (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
					return false;
				}
				std::string newState = strarray[iLevel];
				if (!pSensor->mode_state_template.empty())
				{

					std::string szKey = GetValueTemplateKey(pSensor->mode_state_template);
					if (!szKey.empty())
					{
						root[szKey] = newState;
						szSendValue = JSonToRawString(root);
					}
					else
					{
						Log(LOG_ERROR, "Climate device unhandled mode_state_template (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
						return false;
					}
				}
				else
					szSendValue = newState;
				SendMessage(pSensor->mode_command_topic, szSendValue);
				return true;
			}
			else
			{
				Log(LOG_ERROR, "Climate device does not have mode_command_topic (%s/%s)", DeviceID.c_str(), DeviceName.c_str());
				return false;
			}
		}
	}

	SendMessage(pSensor->command_topic, szSendValue);
	return true;
}

bool MQTT::SetSetpoint(const std::string &DeviceID, const float Temp)
{
	if (m_discovered_sensors.find(DeviceID) == m_discovered_sensors.end())
	{
		return false;
	}
	_tMQTTASensor *pSensor = &m_discovered_sensors[DeviceID];
	if (pSensor->component_type != "climate")
	{
		return false;
	}

	Json::Value root;
	if (!pSensor->temperature_state_template.empty())
	{
		std::string szKey = GetValueTemplateKey(pSensor->temperature_state_template);
		if (!szKey.empty())
			root[szKey] = Temp;
		else
		{
			Log(LOG_ERROR, "Cover device unhandled brightness_value_template (%s/%s)", DeviceID.c_str(), pSensor->name.c_str());
			return false;
		}
	}
	else
		root["target_temp"] = Temp;
	std::string szSendValue = JSonToRawString(root);

	std::string szTopic = pSensor->state_topic;
	if (!pSensor->temperature_command_topic.empty())
		szTopic = pSensor->temperature_command_topic;
	SendMessage(szTopic, szSendValue);
	return true;
}