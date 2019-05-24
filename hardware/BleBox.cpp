#include "stdafx.h"
#include "BleBox.h"
#include "hardwaretypes.h"
#include "../json/json.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../httpclient/HTTPClient.h"

struct STR_DEVICE {
	int			unit;
	std::string api_name;
	std::string name;
	int			deviceID;
	uint8_t		subType;
	int			switchType;
	std::string api_state;
};

#define TOT_DEVICE_TYPES 9

const STR_DEVICE DevicesType[TOT_DEVICE_TYPES] =
{
	{ 0, "switchBox", "Switch Box",pTypeLighting2, sTypeAC, STYPE_OnOff, "relay" },
	{ 1, "shutterBox", "Shutter Box", pTypeLighting2, sTypeAC, STYPE_BlindsPercentageInverted, "shutter" },
	{ 2, "wLightBoxS", "Light Box S", pTypeLighting2, sTypeAC, STYPE_Dimmer, "light" },
	{ 3, "wLightBox", "Light Box", pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, "rgbw" },
	{ 4, "gateBox", "Gate Box", pTypeGeneral, sTypePercentage, 0, "gate" },
	{ 5, "dimmerBox", "Dimmer Box", pTypeLighting2, sTypeAC, STYPE_Dimmer, "dimmer" },
	{ 6, "switchBoxD", "Switch Box D", pTypeLighting2, sTypeAC, STYPE_OnOff, "relay" },
	{ 7, "airSensor", "Air Sensor", pTypeAirQuality, sTypeVoltcraft, 0, "air" },
	{ 8, "tempSensor", "Temp Sensor", pTypeGeneral, sTypeTemperature, 0, "tempsensor" },
};

BleBox::BleBox(const int id, const int pollIntervalsec)
{
	m_HwdID = id;
	m_RGBWisWhiteState = true;
	m_RGBWbrightnessState = 255;
	SetSettings(pollIntervalsec);
}

BleBox::~BleBox()
{
}

bool BleBox::StartHardware()
{
	RequestStart();

	LoadNodes();

	m_thread = std::make_shared<std::thread>(&BleBox::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool BleBox::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}

	m_bIsStarted = false;
	return true;
}

void BleBox::Do_Work()
{
	int sec_counter = m_PollInterval - 1;

	Log(LOG_STATUS, "Worker started...");
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % m_PollInterval == 0)
		{
			GetDevicesState();
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

void BleBox::GetDevicesState()
{
	std::lock_guard<std::mutex> l(m_mutex);

	for (const auto& itt : m_devices)
	{
		std::stringstream sstr;
		sstr << "/api/" << DevicesType[itt.second].api_state << "/state";
		std::string command = sstr.str();

		Json::Value root = SendCommand(itt.first, command, 2);
		if (root.empty())
			continue;

		int IP = IPToUInt(itt.first);
		if (IP != 0)
		{
			switch (itt.second)
			{
			case 0:
			{
				if (DoesNodeExists(root, "state") == false)
					break;

				const bool state = root["state"].asBool();

				SendSwitch(IP, 0, 255, state, 0, DevicesType[itt.second].name);
				break;
			}
			case 1:
			{
				if (DoesNodeExists(root, "shutter") == false)
					break;

				root = root["shutter"];

				if (DoesNodeExists(root, "state") == false)
					break;

				const int state = root["state"].asInt();

				if (DoesNodeExists(root, "currentPos", "position") == false)
					break;

				const int currentPos = root["currentPos"]["position"].asInt();
				const int pos = currentPos;

				bool opened = true;
				if ((state == 2 && pos == 100) || (state == 3))
					opened = false;

				SendSwitch(IP, 0, 255, opened, 100 - pos, DevicesType[itt.second].name);
				break;
			}
			case 2:
			{
				if (DoesNodeExists(root, "light", "currentColor") == false)
					break;

				const std::string currentColor = root["light"]["currentColor"].asString();
				unsigned int hexNumber;
				sscanf(currentColor.c_str(), "%x", &hexNumber);
				int level = (int)(hexNumber / (255.0 / 100.0));

				SendSwitch(IP, 0, 255, level > 0, level, DevicesType[itt.second].name);
				break;
			}
			case 3:
			{
				if (DoesNodeExists(root, "rgbw", "currentColor") == false)
					break;

				const std::string currentColor = root["rgbw"]["currentColor"].asString();
				unsigned int hexNumber;
				sscanf(currentColor.c_str(), "%x", &hexNumber);

				SendRGBWSwitch(IP, 0, 255, hexNumber, true, DevicesType[itt.second].name);
				break;
			}
			case 4:
			{
				if (DoesNodeExists(root, "currentPos") == false)
					break;

				const float level = root["currentPos"].asFloat();

				SendPercentageSensor(IP, 1, 255, level, DevicesType[itt.second].name);
				break;
			}
			case 5:
			{
				if (DoesNodeExists(root, "dimmer", "currentBrightness") == false)
					break;

				const int currentPos = root["dimmer"]["currentBrightness"].asInt();
				int level = (int)(currentPos / (255.0 / 100.0));

				SendSwitch(IP, 0, 255, level > 0, level, DevicesType[itt.second].name);
				break;
			}
			case 6:
			{
				if ((DoesNodeExists(root, "relays") == false) || (!root["relays"].isArray()))
					break;

				Json::Value relays = root["relays"];
				Json::ArrayIndex count = relays.size();
				for (Json::ArrayIndex index = 0; index < count; index++)
				{
					Json::Value relay = relays[index];
					if ((DoesNodeExists(relay, "relay") == false) || (DoesNodeExists(relay, "state") == false))
						break;
					uint8_t relayNumber = (uint8_t)relay["relay"].asInt(); // 0 or 1
					bool currentState = relay["state"].asBool(); // 0 or 1
					//std::string name = DevicesType[itt.second].name + " " + relay["state"].asString();
					SendSwitch(IP, relayNumber, 255, currentState, 0, DevicesType[itt.second].name);
				}

				break;
			}
			case 7:
			{
				if (DoesNodeExists(root, "air") == false)
					break;

				root = root["air"];

				if ((DoesNodeExists(root, "sensors") == false) || (!root["sensors"].isArray()))
					break;

				Json::Value sensors = root["sensors"];
				Json::ArrayIndex count = sensors.size();
				for (Json::ArrayIndex index = 0; index < count; index++)
				{
					Json::Value sensor = sensors[index];
					if ((DoesNodeExists(sensor, "type") == false) || (DoesNodeExists(sensor, "value") == false))
						break;
					uint8_t value = (uint8_t)sensor["value"].asInt();
					std::string type = sensor["type"].asString();

					//TODO - how save IP address ??
					SendAirQualitySensor(IP, index + 1, 255, value, type);
				}

				break;
			}
			case 8:
			{
				if (DoesNodeExists(root, "tempSensor") == false)
					break;

				root = root["tempSensor"];

				if ((DoesNodeExists(root, "sensors") == false) || (!root["sensors"].isArray()))
					break;

				Json::Value sensors = root["sensors"];
				Json::ArrayIndex count = sensors.size();
				for (Json::ArrayIndex index = 0; index < count; index++)
				{
					Json::Value sensor = sensors[index];

					if ((DoesNodeExists(sensor, "value") == false) || (DoesNodeExists(sensor, "state") == false))
						break;

					if (sensor["state"] != 2)
					{
						Log(LOG_ERROR, "temp sensor error!");
						break;
					}

					std::string temperature = sensor["value"].asString(); // xxxx (xx.xx = temperature)
					float ftemp = static_cast<float>(std::stoi(temperature.substr(0, 2)) + std::stoi(temperature.substr(2, 2)) / 100.0);

					//TODO - how save IP address ??
					SendTempSensor(IP, 255, ftemp, DevicesType[itt.second].name);
				}

				break;
			}
			}
			SetHeartbeatReceived();
		}
	}
}

std::string BleBox::GetDeviceIP(const tRBUF * id)
{
	char ip[20];

	sprintf(ip, "%d.%d.%d.%d", id->LIGHTING2.id1, id->LIGHTING2.id2, id->LIGHTING2.id3, id->LIGHTING2.id4);
	return ip;
}

std::string BleBox::GetDeviceRevertIP(const tRBUF * id)
{
	char ip[20];

	sprintf(ip, "%d.%d.%d.%d", id->LIGHTING2.id4, id->LIGHTING2.id3, id->LIGHTING2.id2, id->LIGHTING2.id1);
	return ip;
}

std::string BleBox::GetDeviceIP(const std::string & id)
{
	BYTE id1, id2, id3, id4;
	char ip[20];

	sscanf(id.c_str(), "%2hhx%2hhx%2hhx%2hhx", &id1, &id2, &id3, &id4);
	sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);

	return ip;
}

int BleBox::GetDeviceTypeByApiName(const std::string & apiName)
{
	for (unsigned int i = 0; i < TOT_DEVICE_TYPES; ++i)
	{
		if (DevicesType[i].api_name == apiName)
		{
			return DevicesType[i].unit;
		}
	}
	Log(LOG_ERROR, "unknown device api name(%s)", apiName.c_str());
	return -1;
}

std::string BleBox::IPToHex(const std::string & IPAddress, const int type)
{
	std::vector<std::string> strarray;
	StringSplit(IPAddress, ".", strarray);
	if (strarray.size() != 4)
		return "";

	char szIdx[10];
	// because exists inconsistency when comparing deviceID in method decode_xxx in mainworker(Limitless uses small letter, lighting2 etc uses capital letter)
	if (type != pTypeColorSwitch)
	{
		uint32_t sID = (uint32_t)(atoi(strarray[0].c_str()) << 24) | (uint32_t)(atoi(strarray[1].c_str()) << 16) | (atoi(strarray[2].c_str()) << 8) | atoi(strarray[3].c_str());
		sprintf(szIdx, "%08X", (unsigned int)sID);
	}
	else
	{
		sprintf(szIdx, "%X%02X%02X%02X", atoi(strarray[0].data()), atoi(strarray[1].data()), atoi(strarray[2].data()), atoi(strarray[3].data()));
	}
	return szIdx;
}

bool BleBox::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	const tRBUF* output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeLighting2 && output->LIGHTING2.subtype == sTypeAC)
	{
		std::string IPAddress = GetDeviceIP(output);

		int type = GetDeviceType(IPAddress);

		if (type != -1)
		{
			switch (type)
			{
			case 0:
			{
				std::string state;
				if (output->LIGHTING2.cmnd == light2_sOn)
				{
					state = "1";
				}
				else
				{
					state = "0";
				}

				Json::Value root = SendCommand(IPAddress, "/s/" + state);
				if (root.empty())
					return false;

				if (DoesNodeExists(root, "state") == false)
					return false;

				if (root["state"].asString() != state)
				{
					Log(LOG_ERROR, "state not changed!");
					return false;
				}
				break;
			}

			case 1: // shutterbox
			{
				int percentage = 0;
				switch (output->LIGHTING2.cmnd) {
				case light2_sOn:
					percentage = 0;
					break;
				case light2_sOff:
					percentage = 100;
					break;
				default:
					percentage = 100 - output->LIGHTING2.level * 100 / 15;
					break;
				}

				Json::Value root = SendCommand(IPAddress, "/s/p/" + std::to_string(percentage));

				if (root.empty())
					return false;

				if (DoesNodeExists(root, "shutter") == false)
					return false;

				root = root["shutter"];

				if (DoesNodeExists(root, "desiredPos", "position") == false)
					return false;

				if (root["desiredPos"]["position"].asInt() != percentage)
					return false;

				break;
			}

			case 2:
			{
				std::string level;
				if (output->LIGHTING2.cmnd == light2_sOn)
				{
					level = "ff";
				}
				else
					if (output->LIGHTING2.cmnd == light2_sOff)
					{
						level = "00";
					}
					else
					{
						uint8_t percentage = static_cast<uint8_t>(output->LIGHTING2.level * 255 / 15);

						char value[4];
						sprintf(value, "%x", percentage);
						level = value;
					}

				Json::Value root = SendCommand(IPAddress, "/s/" + level);
				if (root.empty())
					return false;

				if (DoesNodeExists(root, "light", "desiredColor") == false)
					return false;

				if (root["light"]["desiredColor"].asString() != level)
				{
					Log(LOG_ERROR, "light not changed!");
					return false;
				}

				break;
			}

			case 5: // dimmerBox
			{
				std::string level;
				if (output->LIGHTING2.cmnd == light2_sOn)
				{
					level = "ff";
				}
				else
					if (output->LIGHTING2.cmnd == light2_sOff)
					{
						level = "0";
					}
					else
					{
						uint8_t percentage = static_cast<uint8_t>(output->LIGHTING2.level * 255 / 15);

						char value[4];
						sprintf(value, "%x", percentage);
						level = value;
					}

				Json::Value root = SendCommand(IPAddress, "/s/" + level);
				if (root.empty())
					return false;

				if (DoesNodeExists(root, "dimmer", "desiredBrightness") == false)
					return false;

				std::stringstream ss;
				ss << std::hex << root["dimmer"]["desiredBrightness"].asInt();
				std::string state = ss.str();

				if (state != level)
				{
					Log(LOG_ERROR, "dimmer not changed!");
					return false;
				}

				break;
			}

			case 6: //switchboxd
			{
				std::string state;
				std::string relayNumber;
				if (output->LIGHTING2.cmnd == light2_sOn)
				{
					state = "1";
				}
				else
				{
					state = "0";
				}
				if (output->LIGHTING2.unitcode == 0)
				{
					relayNumber = "0";
				}
				else
				{
					relayNumber = "1";
				}

				std::stringstream ss;
				ss << "/s/" << relayNumber << "/" << state;

				Json::Value root = SendCommand(IPAddress, ss.str());
				if (root.empty())
					return false;

				if ((DoesNodeExists(root, "relays") == false) || (!root["relays"].isArray()))
					return false;

				bool success = false;
				Json::Value relays = root["relays"];
				Json::ArrayIndex count = relays.size();
				for (Json::ArrayIndex index = 0; index < count; index++)
				{
					Json::Value relay = relays[index];
					if ((DoesNodeExists(relay, "relay") == false) || (DoesNodeExists(relay, "state") == false))
						continue;
					int relayNumber2 = relay["relay"].asInt(); // 0 or 1
					std::string currentState = relay["state"].asString(); // 0 or 1
					if (((output->LIGHTING2.unitcode) == relayNumber2) && (state == currentState))
					{
						success = true;
						break;
					}
				}
				return success;

				break;
			}
			default:
			{
				return false;
			}
			}
			return true;
		}
	}

	if (output->ICMND.packettype == pTypeGeneralSwitch && output->LIGHTING2.subtype == sTypeAC)
	{
		std::string IPAddress = GetDeviceRevertIP(output);

		int type = GetDeviceType(IPAddress);

		switch (type)
		{
		case 4: // gatebox
		{
			std::string command;
			if (output->ICMND.msg4 == 2) // 2=primary, 3=secondary
			{
				command = "p";
			}
			else
			{
				command = "s";
			}

			Json::Value root = SendCommand(IPAddress, "/s/" + command);
			if (root.empty())
				return false;

			if (DoesNodeExists(root, "gate") == false)
				return false;

			break;
		}
		default:
		{
			return false;
		}

		}
		return true;
	}

	if (output->ICMND.packettype == pTypeColorSwitch && output->LIGHTING2.subtype == sTypeColor_RGB_W)
	{
		std::string IPAddress = GetDeviceRevertIP(output);

		const _tColorSwitch* pLed = reinterpret_cast<const _tColorSwitch*>(pdata);
		int red, green, blue, white;
		bool setColor = true;

		switch (pLed->command)
		{
		case Color_LedOn: {
			if (m_RGBWColorState.mode != ColorModeNone && !m_RGBWisWhiteState)
			{
				red = int(round(m_RGBWColorState.r * m_RGBWbrightnessState / 255.0f));
				green = int(round(m_RGBWColorState.g * m_RGBWbrightnessState / 255.0f));
				blue = int(round(m_RGBWColorState.b * m_RGBWbrightnessState / 255.0f));
				white = 0;
			}
			else
			{
				red = 0;
				green = 0;
				blue = 0;
				white = m_RGBWbrightnessState;
			}
			break;
		}
		case Color_LedOff:
			red = 0;
			green = 0;
			blue = 0;
			white = 0;
			break;
		case Color_SetColorToWhite: {
			m_RGBWisWhiteState = true;
			m_RGBWColorState = pLed->color; //TODO: Is there any point of doing this?
			setColor = false;//Sending is done by SetBrightnessLevel
			break;
		}
		case Color_SetColor: {
			if (pLed->color.mode == ColorModeWhite)
			{
				m_RGBWisWhiteState = true;
				m_RGBWColorState = pLed->color; //TODO: Is there any point of doing this?
			}
			else if (pLed->color.mode == ColorModeRGB)
			{
				m_RGBWisWhiteState = false;
				m_RGBWColorState = pLed->color;
			}
			else {
				Log(LOG_STATUS, "SetRGBColour - Color mode %d is unhandled, if you have a suggestion for what it should do, please post on the Domoticz forum", pLed->color.mode);
			}
			// No break, fall through to send combined color + brightness command
		}
		case Color_SetBrightnessLevel: {
			int BrightnessBase = (int)pLed->value;
			int dMax_Send = (int)(round((255.0f / 100.0f) * float(BrightnessBase)));

			m_RGBWbrightnessState = dMax_Send;

			if (m_RGBWisWhiteState) // TODO: Check m_RGBWColorState.mode instead
			{
				red = 0;
				green = 0;
				blue = 0;
				white = dMax_Send;
			}
			else
			{
				red = int(round(m_RGBWColorState.r * dMax_Send / 255.0f));
				green = int(round(m_RGBWColorState.g * dMax_Send / 255.0f));
				blue = int(round(m_RGBWColorState.b * dMax_Send / 255.0f));
				white = 0;
			}
			break;
		}
		default:
			setColor = false;
			break;
		}

		if (!setColor)
			return false;

		char level[10];
		sprintf(level, "%02x%02x%02x%02x", red, green, blue, white);
		std::string state(level);

		Json::Value root = SendCommand(IPAddress, "/s/" + state);
		if (root.empty())
			return false;

		if (DoesNodeExists(root, "rgbw", "desiredColor") == false)
			return false;

		if (root["rgbw"]["desiredColor"].asString() != state)
		{
			Log(LOG_ERROR, "rgbw not changed!");
			return false;
		}
		return true;
	}

	return false;
}

bool BleBox::DoesNodeExists(const Json::Value & root, const std::string & node)
{
	if (root[node].empty() == true)
	{
		Log(LOG_ERROR, "node '%s' missing!", node.c_str());
		return false;
	}
	return true;
}

bool BleBox::DoesNodeExists(const Json::Value & root, const std::string & node, const std::string & value)
{
	if (DoesNodeExists(root, node) == false)
		return false;

	if (root[node][value].empty() == true)
	{
		Log(LOG_ERROR, "value '%s' missing!", value.c_str());
		return false;
	}
	return true;
}

void BleBox::SetSettings(const int pollIntervalSec)
{
	m_PollInterval = 30;

	if (pollIntervalSec > 0)
		m_PollInterval = pollIntervalSec;
}

void BleBox::SendSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string & defaultname)
{ //TODO - remove this method, when in DomoticzHardware bug is fix (15 instead 16)
	double rlevel = (15.0 / 100.0) * Level;
	uint8_t level = (uint8_t)(rlevel);

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szIdx, ChildID, int(pTypeLighting2), int(sTypeAC));
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][1].c_str());
		if ((!bOn) && (nvalue == light2_sOff))
			return;
		if ((bOn && (nvalue != light2_sOff)))
		{
			//Check Level
			int slevel = atoi(result[0][2].c_str());
			if (slevel == level)
				return;
		}
	}

	//Send as Lighting 2
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = ID1;
	lcmd.LIGHTING2.id2 = ID2;
	lcmd.LIGHTING2.id3 = ID3;
	lcmd.LIGHTING2.id4 = ID4;
	lcmd.LIGHTING2.unitcode = ChildID;
	if (!bOn)
	{
		lcmd.LIGHTING2.cmnd = light2_sOff;
	}
	else
	{
		if (level != 0)
			lcmd.LIGHTING2.cmnd = light2_sSetLevel;
		else
			lcmd.LIGHTING2.cmnd = light2_sOn;
	}
	lcmd.LIGHTING2.level = level;
	lcmd.LIGHTING2.filler = 0;
	lcmd.LIGHTING2.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char*)& lcmd.LIGHTING2, defaultname.c_str(), BatteryLevel);
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_BleBoxGetNodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;

			root["status"] = "OK";
			root["title"] = "BleBoxGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,DeviceID FROM DeviceStatus WHERE (HardwareID=='%d')", pBaseHardware->m_HwdID);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& itt : result)
				{
					std::vector<std::string> sd = itt;

					BYTE id1, id2, id3, id4;
					char ip[20];
					sscanf(sd[2].c_str(), "%2hhx%2hhx%2hhx%2hhx", &id1, &id2, &id3, &id4);
					sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = ip;
					root["result"][ii]["Type"] = "unknown";
					root["result"][ii]["Uptime"] = "unknown";
					root["result"][ii]["hv"] = "unknown";
					root["result"][ii]["fv"] = "unknown";

					BleBox* pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

					int type = pHardware->GetDeviceType(ip);
					if (type != -1)
					{
						root["result"][ii]["Type"] = DevicesType[type].name;

						Json::Value state = pHardware->GetApiDeviceState(ip);
						if (!state.isNull())
						{
							if (pHardware->DoesNodeExists(state, "fv"))
								root["result"][ii]["fv"] = state["fv"].asString();

							if (pHardware->DoesNodeExists(state, "hv"))
								root["result"][ii]["hv"] = state["hv"].asString();

							std::string uptime = pHardware->GetUptime(ip);
							root["result"][ii]["Uptime"] = uptime;
						}
					}

					ii++;
				}
			}
		}

		void CWebServer::Cmd_BleBoxSetMode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string mode1 = request::findValue(&req, "mode1");
			std::string mode2 = request::findValue(&req, "mode2");
			if (
				(hwid == "") ||
				(mode1 == "") ||
				(mode2 == "")
				)
				return;
			CDomoticzHardwareBase * pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1);
			pHardware->Restart();
		}


		void CWebServer::Cmd_BleBoxAddNode(WebEmSession & session, const request & req, Json::Value & root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = request::findValue(&req, "name");
			std::string ip = request::findValue(&req, "ip");
			if (
				(hwid == "") ||
				(name == "") ||
				(ip == "")
				)
				return;
			CDomoticzHardwareBase * pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxAddNode";
			pHardware->AddNode(name, ip, true);
		}

		void CWebServer::Cmd_BleBoxRemoveNode(WebEmSession & session, const request & req, Json::Value & root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			CDomoticzHardwareBase * pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxRemoveNode";
			int ID = atoi(nodeid.c_str());
			pHardware->RemoveNode(ID);
		}

		void CWebServer::Cmd_BleBoxClearNodes(WebEmSession & session, const request & req, Json::Value & root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_BleBoxAutoSearchingNodes(WebEmSession & session, const request & req, Json::Value & root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string ipmask = request::findValue(&req, "ipmask");
			if (
				(hwid == "") ||
				(ipmask == "")
				)
				return;
			CDomoticzHardwareBase * pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxAutoSearchingNodes";
			pHardware->SearchNodes(ipmask);
		}

		void CWebServer::Cmd_BleBoxUpdateFirmware(WebEmSession & session, const request & req, Json::Value & root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox * pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxUpdateFirmware";
			pHardware->UpdateFirmware();
		}

	}
}

Json::Value BleBox::SendCommand(const std::string & IPAddress, const std::string & command, const int timeOut)
{
	std::string result;
	Json::Value root;

	std::stringstream sstr;
	sstr << "http://" << IPAddress << command;

	std::string sURL = sstr.str();
	HTTPClient::SetConnectionTimeout(timeOut);
	HTTPClient::SetTimeout(4);
	if (!HTTPClient::GET(sURL, result))
	{
		Log(LOG_ERROR, "send '%s'command to %s failed!", command.c_str(), IPAddress.c_str());
		return root;
	}

	Json::Reader jReader;
	if (!jReader.parse(result, root))
	{
		Log(LOG_ERROR, "Invalid json received!");
		return root;
	}

	if (root.size() == 0)
	{
		Log(LOG_ERROR, "Json is empty!");
		return root;
	}

	if (root.isArray())
		root = root[0];

	return root;
}

std::string BleBox::IdentifyDevice(const std::string & IPAddress)
{
	Json::Value root = SendCommand(IPAddress, "/api/device/state", 2);
	if (!root.isObject())
		return "";

	std::string result;

	if (root["device"].empty() == true)
	{
		if (DoesNodeExists(root, "type") == false)
			return "";
		else
			result = root["type"].asString();
	}
	else
	{
		if (root["device"]["type"].empty() == true)
		{
			Log(LOG_ERROR, "Invalid device type received!");
			return "";
		}
		result = root["device"]["type"].asString();
	}
	return result;
}

Json::Value BleBox::GetApiDeviceState(const std::string & IPAddress)
{
	Json::Value empty;

	Json::Value root = SendCommand(IPAddress, "/api/device/state", 2);
	if (!root.isObject())
		return empty;

	if (root["device"].empty() == true)
	{
		return root;
	}
	else
	{
		return root["device"];
	}
}

std::string BleBox::GetUptime(const std::string & IPAddress)
{
	Json::Value root = SendCommand(IPAddress, "/api/device/uptime", 2);
	if (root.empty())
	{
		root = SendCommand(IPAddress, "/api/device/runtime", 2);
		if (root.empty())
			return "unknown";
		if (DoesNodeExists(root, "runtime") == false)
			return "unknown";
		root = root["runtime"];
	}

	uint64_t total_minutes = 0;
	if (root["uptime"].empty() == false)
	{
		uint64_t msec = root["uptime"].asUInt64();
		total_minutes = msec / (1000 * 60);
	}
	else if (root["uptimeS"].empty() == false)
	{
		uint64_t sec = root["uptimeS"].asUInt64();
		total_minutes = sec / 60;
	}
	else if (root["timeH"].empty() == false)
	{
		uint64_t h = root["timeH"].asUInt64();
		total_minutes = h * 60;
	}
	else
		return "unknown";

	int days = static_cast<int>(total_minutes / (24 * 60));
	int hours = static_cast<int>(total_minutes / 60 - days * 24);
	int mins = static_cast<int>(total_minutes - days * 24 * 60 - hours * 60);   //sec / 60 - day * (24 * 60) - hour * 60;

	char timestring[32];
	sprintf(timestring, "%d:%02d:%02d", days, hours, mins);

	return timestring;
}

int BleBox::GetDeviceType(const std::string & IPAddress)
{
	std::map<const std::string, const int>::const_iterator itt = m_devices.find(IPAddress);
	if (itt == m_devices.end())
	{
		Log(LOG_ERROR, "unknown device (%s)", IPAddress.c_str());
		return -1;
	}
	else
	{
		return itt->second;
	}
}

void BleBox::AddNode(const std::string & name, const std::string & IPAddress, bool reloadNodes)
{
	std::string deviceApiName = IdentifyDevice(IPAddress);
	if (deviceApiName.empty())
		return;

	int deviceTypeID = GetDeviceTypeByApiName(deviceApiName);
	if (deviceTypeID == -1)
		return;

	STR_DEVICE deviceType = DevicesType[deviceTypeID];

	std::string szIdx = IPToHex(IPAddress, deviceType.deviceID);

	if (deviceType.unit == 4) // gatebox
	{
		m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 1, (uint8_t)deviceType.deviceID, deviceType.subType, deviceType.switchType, 0, "Unavailable", name);
		m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 2, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, 0, "Unavailable", name);
		m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 3, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, 0, "Unavailable", name);
	}
	else
		if (deviceType.unit == 6) // switchboxd
		{
			m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 0, (uint8_t)deviceType.deviceID, deviceType.subType, deviceType.switchType, 0, "Unavailable", name);
			m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 1, (uint8_t)deviceType.deviceID, deviceType.subType, deviceType.switchType, 0, "Unavailable", name);
		}
		else
		{
			m_sql.InsertDevice(m_HwdID, szIdx.c_str(), 0, (uint8_t)deviceType.deviceID, deviceType.subType, deviceType.switchType, 0, "Unavailable", name);
		}

	if (reloadNodes)
	{
		ReloadNodes();
	}
}

void BleBox::RemoveNode(const int id)
{
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (ID=='%d')", m_HwdID, id);

	ReloadNodes();
}

void BleBox::RemoveAllNodes()
{
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);

	UnloadNodes();
}

void BleBox::UnloadNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);

	m_devices.clear();
}

bool BleBox::LoadNodes()
{
	std::lock_guard<std::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,DeviceID FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		for (const auto& itt : result)
		{
			const std::vector<std::string>& sd = itt;
			std::string addressIP = GetDeviceIP(sd[1]);

			std::string deviceApiName = IdentifyDevice(addressIP);
			if (deviceApiName.empty())
				continue;

			int deviceTypeID = GetDeviceTypeByApiName(deviceApiName);
			if (deviceTypeID == -1)
				continue;

			m_devices.insert(std::pair<const std::string, const int>(addressIP, deviceTypeID));
		}
		return true;
	}
	else
	{
		Log(LOG_ERROR, "Cannot find any devices...");
		return false;
	}
}

void BleBox::ReloadNodes()
{
	UnloadNodes();
	LoadNodes();
}

void BleBox::UpdateFirmware()
{
	for (const auto& itt : m_devices)
	{
		Json::Value root = SendCommand(itt.first, "/api/ota/update", 2);
	}
}

void BleBox::SearchNodes(const std::string & ipmask)
{
	std::vector<std::string> strarray;
	StringSplit(ipmask, ".", strarray); // ipmask - expected "x.y.z.*"
	if (strarray.size() != 4)
		return;
	if (strarray[3] != "*")
		return;
	if (!isInt(strarray[0]) || !isInt(strarray[1]) || !isInt(strarray[2]))
		return;

	std::vector<std::thread> searchingThreads;

	std::stringstream sstr;
	sstr << strarray[0] << "." << strarray[1] << "." << strarray[2] << ".";
	const std::string ipStart = sstr.str();

	for (unsigned int i = 1; i < 255; ++i)
	{
		std::string IPAddress = ipStart + std::to_string(i);

		if (m_devices.find(IPAddress) == m_devices.end())
		{
			searchingThreads.emplace_back(&BleBox::AddNode, this, "unknown", IPAddress, false);
		}
	}

	for (auto& thread : searchingThreads)
	{
		thread.join();
	}

	ReloadNodes();
}
