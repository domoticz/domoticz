#include "stdafx.h"
#include "BleBox.h"
#include "hardwaretypes.h"
#include "../main/json_helper.h"
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../httpclient/HTTPClient.h"

BleBox::BleBox(const int id, const int pollIntervalsec, int showRSSI)
{
	m_HwdID = id;

	SetSettings(pollIntervalsec, showRSSI);
}

bool BleBox::StartHardware()
{
	RequestStart();

	ReloadNodes();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
			m_LastHeartbeat = mytime(nullptr);
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

	for (const auto& device : m_devices)
	{

		std::stringstream sstr;
		sstr << "/api/" << DevicesType[device.bleBoxTypeId].api_state << "/state";
		std::string command = sstr.str();

		Json::Value root = SendCommand(device.addressIP, command, 2);
		if (root.empty())
			continue;

		int deviceID = IPToInt(device.addressIP);

		int signalQuality = 12;
		if (m_ShowRSSI == 2)
		{
			int rssi = GetRSSI(device.addressIP);
			if (rssi != 0)
			{
				signalQuality = RSSIToQuality(rssi);
			}
		}


		switch (device.bleBoxTypeId)
		{
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

			SendSwitch(deviceID, 0, 255, opened, 100 - pos, DevicesType[device.bleBoxTypeId].name, signalQuality);
			break;
		}
		case 2:
		{
			if (DoesNodeExists(root, "light", "currentColor") == false)
				break;

			const std::string currentColor = root["light"]["currentColor"].asString();
			unsigned int hexNumber;
			sscanf(currentColor.c_str(), "%x", &hexNumber);
			auto level = (int)(hexNumber / (255.0 / 100.0));

			SendSwitch(deviceID, 0, 255, level > 0, level, DevicesType[device.bleBoxTypeId].name, signalQuality);
			break;
		}
		case 3:
		{
			if (DoesNodeExists(root, "rgbw", "currentColor") == false)
				break;

			const std::string currentColor = root["rgbw"]["currentColor"].asString();
			unsigned int hexNumber;
			sscanf(currentColor.c_str(), "%x", &hexNumber);

			SendRGBWSwitch(deviceID, 0, 255, hexNumber, true, DevicesType[device.bleBoxTypeId].name, m_Name);
			break;
		}
		case 4:
		{
			if (DoesNodeExists(root, "currentPos") == false)
				break;

			const float level = root["currentPos"].asFloat();

			SendPercentageSensor(deviceID, 1, 255, level, DevicesType[device.bleBoxTypeId].name);
			break;
		}
		case 5:
		{
			if (DoesNodeExists(root, "dimmer", "currentBrightness") == false)
				break;

			const int currentPos = root["dimmer"]["currentBrightness"].asInt();
			auto level = (int)(currentPos / (255.0 / 100.0));

			SendSwitch(deviceID, 0, 255, level > 0, level, DevicesType[device.bleBoxTypeId].name, signalQuality);
			break;
		}
		case 0:
		case 6:
		{
			if ((DoesNodeExists(root, "relays") == false) || (!root["relays"].isArray()))
				break;

			for (const auto& relay : root["relays"])
			{
				if ((DoesNodeExists(relay, "relay") == false) || (DoesNodeExists(relay, "state") == false))
					break;
				auto relayNumber = (uint8_t)relay["relay"].asInt(); // 0 or 1
				bool currentState = relay["state"].asBool(); // 0 or 1
				// std::string name = DevicesType[device.second].name + " " + relay["state"].asString();
				SendSwitch(deviceID, relayNumber, 255, currentState, 0, DevicesType[device.bleBoxTypeId].name, signalQuality);
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
				auto value = (uint8_t)sensor["value"].asInt();
				std::string type = sensor["type"].asString();

				// TODO - how save IP address ??
				SendAirQualitySensor(deviceID, index + 1, 255, value, type);
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

			for (const auto& sensor : root["sensors"])
			{
				if ((DoesNodeExists(sensor, "value") == false)
					|| (DoesNodeExists(sensor, "state") == false))
					break;

				if (sensor["state"] != 2)
				{
					Log(LOG_ERROR, "temp sensor error!");
					break;
				}

				std::string temperature = sensor["value"].asString(); // xxxx (xx.xx = temperature)
				auto ftemp = static_cast<float>(std::stoi(temperature.substr(0, 2))
					+ std::stoi(temperature.substr(2, 2)) / 100.0);

				// TODO - how save IP address ??
				SendTempSensor(deviceID, 255, ftemp, DevicesType[device.bleBoxTypeId].name, signalQuality);
			}

			break;
		}
		}
	}
}

std::string BleBox::GetDeviceIP(const tRBUF* id)
{
	return std_format("%d.%d.%d.%d", id->LIGHTING2.id1, id->LIGHTING2.id2, id->LIGHTING2.id3, id->LIGHTING2.id4);
}

std::string BleBox::GetDeviceRevertIP(const tRBUF* id)
{
	return std_format("%d.%d.%d.%d", id->LIGHTING2.id4, id->LIGHTING2.id3, id->LIGHTING2.id2, id->LIGHTING2.id1);
}

uint8_t BleBox::GetDeviceTypeByApiName(const std::string& apiName)
{
	for (const auto& i : DevicesType)
		if (i.api_name == apiName)
			return i.bleBoxTypeId;

	Log(LOG_ERROR, "unknown device api name(%s)", apiName.c_str());
	return 255;
}

bool BleBox::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	const auto output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeLighting2 && output->LIGHTING2.subtype == sTypeAC)
	{
		std::string IPAddress = GetDeviceIP(output);

		int type = GetDeviceType(IPAddress);

		if (type != -1)
		{
			switch (type)
			{
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
						auto percentage = static_cast<uint8_t>(output->LIGHTING2.level * 255 / 15);
						level = std_format("%x", percentage);
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
						auto percentage = static_cast<uint8_t>(output->LIGHTING2.level * 255 / 15);
						level = std_format("%x", percentage);
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

			case 0: // switchbox
			case 6: // switchboxd
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
				red = int(round(m_RGBWColorState.r * m_RGBWbrightnessState / 255.0F));
				green = int(round(m_RGBWColorState.g * m_RGBWbrightnessState / 255.0F));
				blue = int(round(m_RGBWColorState.b * m_RGBWbrightnessState / 255.0F));
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
			auto BrightnessBase = (int)pLed->value;
			auto dMax_Send = (int)(round((255.0F / 100.0F) * float(BrightnessBase)));

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
				red = int(round(m_RGBWColorState.r * dMax_Send / 255.0F));
				green = int(round(m_RGBWColorState.g * dMax_Send / 255.0F));
				blue = int(round(m_RGBWColorState.b * dMax_Send / 255.0F));
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

		std::string state = std_format("%02x%02x%02x%02x", red, green, blue, white);

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

bool BleBox::DoesNodeExists(const Json::Value& root, const std::string& node)
{
	if (root[node].empty() == true)
	{
		Log(LOG_ERROR, "node '%s' missing!", node.c_str());
		return false;
	}
	return true;
}

bool BleBox::DoesNodeExists(const Json::Value& root, const std::string& node, const std::string& value)
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

void BleBox::SetSettings(const int pollIntervalSec, const int showRSSI)
{
	m_PollInterval = 60;

	if (pollIntervalSec > 0)
		m_PollInterval = pollIntervalSec;

	m_ShowRSSI = showRSSI;
}

void BleBox::SendSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string& defaultname, const BYTE rssi)
{ //TODO - remove this method, when in DomoticzHardware bug is fix (15 instead 16)
	double rlevel = (15.0 / 100.0) * Level;
	uint8_t level = (uint8_t)(rlevel);

	//make device ID
	auto ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	auto ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	auto ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	auto ID4 = (unsigned char)NodeID & 0xFF;

	std::string szIdx = std_format("%X%02X%02X%02X");
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx.c_str(),
		ChildID, int(pTypeLighting2), int(sTypeAC));
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
	lcmd.LIGHTING2.rssi = rssi;
	sDecodeRXMessage(this, (const unsigned char*)&lcmd.LIGHTING2, defaultname.c_str(), BatteryLevel, m_Name.c_str());
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
			if (pBaseHardware == nullptr)
				return;

			root["status"] = "OK";
			root["title"] = "BleBoxGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Name, StrParam1, StrParam2 FROM DeviceStatus WHERE (HardwareID=='%d')", pBaseHardware->m_HwdID);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto& sd : result)
				{
					std::string ip = sd[3];

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = ip;
					root["result"][ii]["Type"] = "unknown";
					root["result"][ii]["Uptime"] = "unknown";
					root["result"][ii]["hv"] = "unknown";
					root["result"][ii]["fv"] = "unknown";
					root["result"][ii]["rssi"] = "unknown";

					BleBox* pHardware = dynamic_cast<BleBox*>(pBaseHardware);

					int type = stoi(sd[2]);
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

							int rssi = pHardware->GetRSSI(ip);
							if (rssi != 0)
							{
								root["result"][ii]["rssi"] = std::to_string(rssi) + " dBm";
							}
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
			if ((hwid.empty()) || (mode1.empty()) || (mode2.empty()))
				return;
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			BleBox* pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1, iMode2);
			pHardware->Restart();
		}


		void CWebServer::Cmd_BleBoxAddNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string ip = HTMLSanitizer::Sanitize(request::findValue(&req, "ip"));
			if ((hwid.empty()) || (name.empty()) || (ip.empty()))
				return;
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			auto pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxAddNode";
			pHardware->AddNode(name, ip, true);
		}

		void CWebServer::Cmd_BleBoxRemoveNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if ((hwid.empty()) || (nodeid.empty()))
				return;
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			BleBox* pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxRemoveNode";
			int ID = atoi(nodeid.c_str());
			pHardware->RemoveNode(ID);
		}

		void CWebServer::Cmd_BleBoxClearNodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			auto pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxClearNodes";
			pHardware->RemoveAllNodes();
		}

		void CWebServer::Cmd_BleBoxAutoSearchingNodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string ipmask = request::findValue(&req, "ipmask");
			if ((hwid.empty()) || (ipmask.empty()))
				return;
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			BleBox* pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxAutoSearchingNodes";
			pHardware->SearchNodes(ipmask);
		}

		void CWebServer::Cmd_BleBoxUpdateFirmware(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == nullptr)
				return;
			auto pHardware = dynamic_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxUpdateFirmware";
			pHardware->UpdateFirmware();
		}

	} // namespace server
} // namespace http

Json::Value BleBox::SendCommand(const std::string& IPAddress, const std::string& command, const int timeOut)
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

	if (!ParseJSon(result, root))
	{
		Log(LOG_ERROR, "(IP: %s, Command: %s) Invalid json received!", IPAddress.c_str(), command.c_str());
		return root;
	}

	if (root.empty())
	{
		Log(LOG_ERROR, "(IP: %s, Command: %s) Json is empty!", IPAddress.c_str(), command.c_str());
		return root;
	}

	if (root.isArray())
		root = root[0];

	return root;
}

std::pair<std::string, std::string> BleBox::IdentifyDevice(const std::string& IPAddress)
{
	Json::Value root = SendCommand(IPAddress, "/info", 2);
	if (!root.isObject())
		return std::make_pair("", "");

	std::pair<std::string, std::string> result;

	if (root["device"].empty() == true)
	{
		if (!DoesNodeExists(root, "type"))
			return std::make_pair("", "");

		auto type = root["type"].asString();
		auto name = root["device"]["deviceName"].asString();
		result = std::make_pair(type, name);
	}
	else
	{
		if (root["device"]["type"].empty() == true)
		{
			Log(LOG_ERROR, "Invalid device type received!");
			return std::make_pair("", "");
		}
		auto type = root["device"]["type"].asString();
		auto name = root["device"]["deviceName"].asString();
		result = std::make_pair(type, name);
	}
	return result;
}

Json::Value BleBox::GetApiDeviceState(const std::string& IPAddress)
{
	Json::Value empty;

	Json::Value root = SendCommand(IPAddress, "/info", 2);
	if (!root.isObject())
		return empty;

	if (root["device"].empty() == true)
	{
		return root;
	}

	return root["device"];
}

std::string BleBox::GetUptime(const std::string& IPAddress)
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

	auto days = static_cast<int>(total_minutes / (24 * 60));
	auto hours = static_cast<int>(total_minutes / 60 - days * 24);
	auto mins = static_cast<int>(total_minutes - days * 24 * 60 - hours * 60);   //sec / 60 - day * (24 * 60) - hour * 60;

	return std_format("%d:%02d:%02d", days, hours, mins);
}

int BleBox::GetRSSI(const std::string& IPAddress)
{
	if (m_ShowRSSI == 0)
	{
		return 0;
	}

	Json::Value root = SendCommand(IPAddress, "/api/device/network", 2);
	if (root.empty())
	{
		return 0;
	}

	if (root["ssid"].empty())
	{
		return 0;
	}

	auto ssid = root["ssid"].asString();

	root = SendCommand(IPAddress, "/api/wifi/scan", 2);
	if (root.empty() || DoesNodeExists(root, "ap") == false)
	{
		return 0;
	}

	for (const auto& ap : root["ap"])
	{
		if (DoesNodeExists(ap, "ssid") == true)
		{
			if (ap["ssid"].asString() == ssid)
			{
				if (DoesNodeExists(ap, "rssi") == true)
				{
					auto rssi = ap["rssi"].asInt();
					return rssi;
				}
			}
		}
	}

	return 0;
}

int BleBox::GetDeviceType(const std::string& IPAddress)
{
	auto itt = std::find_if(m_devices.begin(), m_devices.end(), [&IPAddress](const _tBleBoxDevice& obj) {
		return obj.addressIP == IPAddress;
		});

	if (itt == m_devices.end())
	{
		Log(LOG_ERROR, "unknown device (%s)", IPAddress.c_str());
		return -1;
	}

	return itt->bleBoxTypeId;
}

void BleBox::AddNode(const std::string& name, const std::string& IPAddress, bool reloadNodes)
{
	auto itt = std::find_if(m_devices.begin(), m_devices.end(), [&IPAddress](const _tBleBoxDevice& obj) {
		return obj.addressIP == IPAddress;
		});

	if (itt != m_devices.end())
		return;


	auto device = IdentifyDevice(IPAddress);
	std::string deviceApiName = device.first;
	if (deviceApiName.empty())
		return;

	uint8_t deviceTypeID = GetDeviceTypeByApiName(deviceApiName);
	if (deviceTypeID == 255)
		return;

	if (!m_sql.m_bAcceptNewHardware)
	{
		Log(LOG_NORM, "Accept new hardware disabled. Ignoring new sensor %s", IPAddress.c_str());
		return;
	}

	auto deviceName = name;
	if (deviceName == "")
	{
		deviceName = device.second;
	}

	const auto& deviceType = DevicesType[deviceTypeID];
	auto ip = IPToInt(IPAddress);
	auto address = CodeID(ip, deviceType.rfxType);
	auto id = address.first.c_str();
	auto unit = address.second;

	int signalQuality = 12;
	if (m_ShowRSSI == 2)
	{
		int rssi = GetRSSI(IPAddress);
		if (rssi != 0)
		{
			signalQuality = RSSIToQuality(rssi);
		}
	}

	if (deviceType.bleBoxTypeId == 4) // gatebox
	{
		auto idx = m_sql.InsertDevice(m_HwdID, 0, id, 1, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", deviceName, signalQuality, 255, 0);
		_log.Debug(DEBUG_HARDWARE, "BleBox: new device Id: '%d'", idx);
		m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);

		idx = m_sql.InsertDevice(m_HwdID, 0, id, 2, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, 0, "Unavailable", deviceName, signalQuality, 255, 0);
		m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);

		idx = m_sql.InsertDevice(m_HwdID, 0, id, 3, pTypeGeneralSwitch, sTypeAC, STYPE_PushOn, 0, "Unavailable", deviceName, signalQuality, 255, 0);
		m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);
	}
	else
		if (deviceType.bleBoxTypeId == 6) // switchboxd
		{
			auto idx = m_sql.InsertDevice(m_HwdID, 0, id, 0, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", deviceName, signalQuality, 255, 0);
			_log.Debug(DEBUG_HARDWARE, "BleBox: new device Id: '%d'", idx);
			m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);

			idx = m_sql.InsertDevice(m_HwdID, 0, id, 1, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", deviceName, signalQuality, 255, 0);
			m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);
		}
		else
			if (deviceType.bleBoxTypeId == 7) // airbox
			{
				auto idx = m_sql.InsertDevice(m_HwdID, 0, id, 1, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", "Unknown", signalQuality, 255, 0);
				_log.Debug(DEBUG_HARDWARE, "BleBox: new device Id: '%d'", idx);
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);

				idx = m_sql.InsertDevice(m_HwdID, 0, id, 2, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", "Unknown", signalQuality, 255, 0);
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);

				idx = m_sql.InsertDevice(m_HwdID, 0, id, 3, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", "Unknown", signalQuality, 255, 0);
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);
			}
			else
			{
				auto idx = m_sql.InsertDevice(m_HwdID, 0, id, unit, deviceType.rfxType, deviceType.subType, deviceType.switchType, 0, "Unavailable", deviceName, signalQuality, 255, 0);
				_log.Debug(DEBUG_HARDWARE, "BleBox: new device Id: '%d'", idx);
				m_sql.safe_query("UPDATE DeviceStatus SET StrParam1='%d', StrParam2='%q' WHERE (ID==%d)", deviceTypeID, IPAddress.c_str(), idx);
			}

	if (reloadNodes)
	{
		ReloadNodes();
	}
}

void BleBox::RemoveNode(const int id)
{
	m_sql.DeleteDevices(std::to_string(id));

	ReloadNodes();
}

void BleBox::RemoveAllNodes()
{
	auto result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		std::string devices;
		for (const auto& sd : result)
		{
			if (!devices.empty())
				devices += ";";
			devices += sd[0];
		}
		m_sql.DeleteDevices(devices);

		UnloadNodes();
	}
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
	result = m_sql.safe_query("SELECT StrParam1, StrParam2 FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			std::string addressIP = sd[1];
			uint8_t bleBoxTypeID = std::stoi(sd[0]);

			m_devices.push_back({ bleBoxTypeID, addressIP });
		}
		return true;
	}

	Log(LOG_ERROR, "Cannot find any devices...");
	return false;
}

void BleBox::ReloadNodes()
{
	UnloadNodes();
	LoadNodes();
}

void BleBox::UpdateFirmware()
{
	for (const auto& device : m_devices)
	{
		Json::Value root = SendCommand(device.addressIP, "/api/ota/update", 2);
	}
}

void BleBox::SearchNodes(const std::string& pattern)
{
	std::vector<std::string> hosts;
	if (!PrepareHostList(pattern, hosts))
	{
		Log(LOG_ERROR, "Invalid or unsupported IP pattern : %s (expected e.g. 192.168.1.*)", pattern.c_str());
		return;
	}

	std::vector<std::thread> searchingThreads;
	for (auto&& host : hosts)
	{
		auto itt = std::find_if(m_devices.begin(), m_devices.end(), [&host](const _tBleBoxDevice& obj) {
			return obj.addressIP == host;
			});

		if (itt == m_devices.end())
			searchingThreads.emplace_back([this, &host] { AddNode("", host, false); });
	}

	for (auto&& thread : searchingThreads)
	{
		thread.join();
	}

	ReloadNodes();
}

bool BleBox::PrepareHostList(const std::string& pattern, std::vector<std::string>& hosts)
{
	std::vector<std::string> strarray;
	StringSplit(pattern, ".", strarray);

	if (strarray.size() != 4)
		return false;

	if (strarray[3] != "*")
		return false;

	if (!isInt(strarray[0]) || !isInt(strarray[1]) || !isInt(strarray[2]))
		return false;

	std::stringstream sstr;
	sstr << strarray[0] << "." << strarray[1] << "." << strarray[2] << ".";
	const std::string ipStart = sstr.str();

	for (unsigned int i = 1; i < 255; ++i)
	{
		std::string host = ipStart + std::to_string(i);
		hosts.push_back(host);
	}

	return !hosts.empty();
}

std::pair<std::string, uint8_t> BleBox::CodeID(const int id, const uint8_t type)
{
	// because exists inconsistency when comparing deviceID in method decode_xxx in mainworker(Limitless uses small letter, lighting2 etc uses capital letter)

	if (type == pTypeGeneral)
	{
		auto deviceId = std_format("%08X", id);
		auto result = std::make_pair(deviceId, 0);
		return result;
	}

	if (type == pTypeTEMP)
	{
		auto ID3 = (id & 0xFF00) >> 8;
		auto ID4 = id & 0xFF;

		char szTmp[10];
		sprintf(szTmp, "%d", (ID3 * 256) + ID4);
		std::string deviceId = szTmp;
		uint8_t unit = ID4;

		auto result = std::make_pair(deviceId, unit);
		return result;
	}

	if (type == pTypeAirQuality)
	{
		auto ID1 = id & 0xFF;

		char szTmp[10];
		sprintf(szTmp, "%d", ID1);
		std::string deviceId = szTmp;
		uint8_t unit = 0;

		auto result = std::make_pair(deviceId, unit);
		return result;
	}

	if (type == pTypeLighting2)
	{
		auto ID1 = (unsigned char)((id & 0xFF000000) >> 24);
		auto ID2 = (unsigned char)((id & 0xFF0000) >> 16);
		auto ID3 = (unsigned char)((id & 0xFF00) >> 8);
		auto ID4 = (unsigned char)id & 0xFF;

		auto deviceId = std_format("%X%02X%02X%02X", ID1, ID2, ID3, ID4);
		auto result = std::make_pair(deviceId, 0);
		return result;
	}

	auto deviceId = std_format("%d", id);
	auto result = std::make_pair(deviceId, 0);
	return result;
}

int BleBox::IPToInt(const std::string& IPAddress)
{
	std::vector<std::string> strarray;
	StringSplit(IPAddress, ".", strarray);
	if (strarray.size() != 4)
		return 0;

	auto ip = (atoi(strarray[0].data()) << 24) | (atoi(strarray[1].data()) << 16) | (atoi(strarray[2].data()) << 8) | atoi(strarray[3].data());

	return ip;
}

int BleBox::RSSIToQuality(const int rssi)
{
	int quality;
	if (rssi <= -100)
	{
		quality = 0;
	}
	else if (rssi >= -50)
	{
		quality = 100;
	}
	else
	{
		quality = 2 * (rssi + 100);
	}

	return quality / 10;
}