#include "stdafx.h"
#include "BleBox.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "../main/Helper.h"
#include "../main/WebServer.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"

#define TOT_TYPE 6

const _STR_DEVICE DevicesType[TOT_TYPE] =
{ 
	{ 0, "switchBox", "Switch Box",int(pTypeLighting2), int(sTypeAC), int(STYPE_OnOff), "relay" },
	{ 1, "shutterBox", "Shutter Box", int(pTypeLighting2), int(sTypeAC), int(STYPE_BlindsPercentageInverted), "shutter" },
	{ 2, "wLightBoxS", "Light Box S", int(pTypeLighting2), int(sTypeAC), int(STYPE_Dimmer), "light" },
	{ 3, "wLightBox", "Light Box", int(pTypeLimitlessLights), int(sTypeLimitlessRGBW), int(STYPE_Dimmer), "rgbw" },
	{ 4, "gateBox", "Gate Box", int(pTypeLighting2), int(sTypeAC), int(STYPE_Dimmer), "gate" },
	{ 5, "dimmerBox", "Dimmer Box", int(pTypeLighting2), int(sTypeAC), int(STYPE_Dimmer), "dimmer" }
};

int BleBox::GetDeviceTypeByApiName(const std::string &apiName)
{
	for (unsigned int i = 0; i < TOT_TYPE; ++i)
	{
		if (DevicesType[i].api_name == apiName)
		{
			return DevicesType[i].unit;
		}
	}
	_log.Log(LOG_ERROR, "BleBox: unknown device api name({0})", apiName.c_str());
	return -1;
}

BleBox::BleBox(const int id, const int pollIntervalsec) :
	m_stoprequested(false)
{
	_log.Log(LOG_STATUS, "BleBox: Create instance");
	m_HwdID = id;
	SetSettings(pollIntervalsec);
}

BleBox::~BleBox()
{
	_log.Log(LOG_STATUS, "BleBox: Destroy instance");
}

bool BleBox::StartHardware()
{
	if (LoadNodes())
	{
		m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&BleBox::Do_Work, this)));
		m_bIsStarted = true;
		sOnConnected(this);
		return (m_thread != NULL);
	}
	return false;
}

bool BleBox::StopHardware()
{
	m_stoprequested = true;

	if (m_thread)
	{
		m_thread->join();
	}

	m_bIsStarted = false;
	return true;
}

void BleBox::Do_Work()
{
	int sec_counter = m_PollInterval - 1;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % m_PollInterval == 0)
		{
			GetDevicesState();
		}
	}
}

void BleBox::GetDevicesState()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::map<const std::string, const int>::const_iterator itt;
	for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
	{
		std::stringstream sstr;
		sstr << "/api/" << DevicesType[itt->second].api_state << "/state";
		std::string command = sstr.str();

		Json::Value root = SendCommand(itt->first, command);
		if (root == "")
			continue;


		int node = IPToUInt(itt->first);
		if (node != 0)
		{
			switch (itt->second)
			{
				case 0:
				{
					if (root["state"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'state' missing!");
						break;
					}
					const bool state = root["state"].asBool();

					SendSwitch(node, itt->second, 255, state, 0, DevicesType[itt->second].name);
					break;
				}
				case 1:
				{
					if (root["state"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'state' missing!");
						break;
					}
					const int state = root["state"].asInt();

					const int currentPos = root["currentPos"].asInt();
					//	const int desiredPos = root["desiredPos"].asInt();
					const int pos = currentPos;

					bool opened = true;
					if ((state == 2 && pos == 100) || (state == 3))
						opened = false;

					SendSwitch(node, itt->second, 255, opened, pos, DevicesType[itt->second].name);
					break;
				}
				case 2:
				{
					if (root["light"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'light' missing!");
						break;
					}
					if (root["light"]["currentColor"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'currentColor' missing!");
						break;
					}
					const std::string currentColor = root["light"]["currentColor"].asString();
					int hexNumber;
					sscanf(currentColor.c_str(), "%x", &hexNumber);
					int level = (int)(hexNumber / (255.0 / 100.0));

					SendSwitch(node, itt->second, 255, level > 0, level, DevicesType[itt->second].name);
					break;
				}
				case 3:
				{
					if (root["rgbw"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'rgbw' missing!");
						break;
					}
					if (root["rgbw"]["currentColor"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'currentColor' missing!");
						break;
					}
					const std::string currentColor = root["rgbw"]["currentColor"].asString();
					int hexNumber;
					sscanf(currentColor.c_str(), "%x", &hexNumber);

					SendRGBWSwitch(node, itt->second, 255, hexNumber, true, DevicesType[itt->second].name);
					break;
				}
				case 4:
				{
					if (root["currentPos"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'currentPos' missing!");
						break;
					}
					const int currentPos = root["currentPos"].asInt();
					int level = (int)(currentPos / (255.0 / 100.0));

					SendSwitch(node, itt->second, 255, level > 0, level, DevicesType[itt->second].name);
					break;
				}
				case 5:
				{
					if (root["currentBrightness"].empty() == true)
					{
						_log.Log(LOG_ERROR, "BleBox: node 'currentBrightness' missing!");
						break;
					}
					const int currentPos = root["currentBrightness"].asInt();
					int level = (int)(currentPos / (255.0 / 100.0));

					SendSwitch(node, itt->second, 255, level > 0, level, DevicesType[itt->second].name);
					break;
				}
			}
			SetHeartbeatReceived();
		}
	}
}

std::string BleBox::GetDeviceIP(const tRBUF *id)
{
	char ip[20];

	sprintf(ip, "%d.%d.%d.%d", id->LIGHTING2.id1, id->LIGHTING2.id2, id->LIGHTING2.id3, id->LIGHTING2.id4);
	return ip;
}

std::string BleBox::GetDeviceRevertIP(const tRBUF *id)
{
	char ip[20];

	sprintf(ip, "%d.%d.%d.%d", id->LIGHTING2.id4, id->LIGHTING2.id3, id->LIGHTING2.id2, id->LIGHTING2.id1);
	return ip;
}

std::string BleBox::GetDeviceIP(const std::string &id)
{
	BYTE id1, id2, id3, id4;
	char ip[20];

	sscanf(id.c_str(), "%2hhx%2hhx%2hhx%2hhx", &id1, &id2, &id3, &id4);
	sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);

	return ip;
}

std::string BleBox::IPToHex(const std::string &IPAddress, const int type)
{
	std::vector<std::string> strarray;
	StringSplit(IPAddress, ".", strarray);
	if (strarray.size() != 4)
		return "";

	char szIdx[10];
	// because exists inconsistency when comparing deviceID in method decode_xxx in mainworker(Limitless uses small letter, lighting2 etc uses capital letter)
	if (type != pTypeLimitlessLights)
	{ 
		sprintf(szIdx, "%02X%02X%02X%02X", atoi(strarray[0].data()), atoi(strarray[1].data()), atoi(strarray[2].data()), atoi(strarray[3].data()));
	}
	else
	{
		sprintf(szIdx, "%02x%02x%02x%02x", atoi(strarray[0].data()), atoi(strarray[1].data()), atoi(strarray[2].data()), atoi(strarray[3].data()));
	}
	return szIdx;
}

bool BleBox::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeLighting2 && output->LIGHTING2.subtype == sTypeAC)
	{
		std::string IPAddress = GetDeviceIP(output);

		switch (output->LIGHTING2.unitcode)
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
				if (root == "")
					return false;

				if (root["state"].empty() == true)
				{
					_log.Log(LOG_ERROR, "BleBox: node 'state' missing!");
					return false;
				}

				if (root["state"].asString() != state)
				{
					_log.Log(LOG_ERROR, "BleBox: state not changed!");
					return false;
				}
				break;
			}

			case 1:
			{
				std::string state;
				if (output->LIGHTING2.cmnd == light2_sOn)
				{
					state = "u";
				}
				else
					if (output->LIGHTING2.cmnd == light2_sOff)
					{
						state = "d";
					}
					else
					{
						int percentage = output->LIGHTING2.level * 100 / 15;
						state = boost::to_string(percentage);
					}

				Json::Value root = SendCommand(IPAddress, "/s/" + state);
				if (root == "")
					return false;

				if (root["state"].empty() == true)
				{
					_log.Log(LOG_ERROR, "BleBox: node 'state' missing!");
					return false;
				}

				//if (root["state"].asString() != state)
				//{
				//	_log.Log(LOG_ERROR, "BleBox: state not changed!");
				//	return false;
				//}
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
						int percentage = output->LIGHTING2.level * 255 / 15;

						char value[4];
						sprintf(value, "%x", percentage);
						level = value;
					}

				Json::Value root = SendCommand(IPAddress, "/s/" + level);
				if (root == "")
					return false;

				if (root["light"].empty() == true)
				{
					_log.Log(LOG_ERROR, "BleBox: node 'light' missing!");
					return false;
				}
				if (root["light"]["currentColor"].empty() == true)
				{
					_log.Log(LOG_ERROR, "BleBox: node 'currentColor' missing!");
					return false;
				}

				if (root["light"]["currentColor"].asString() != level) // TODO or desiredcolor ??
				{
					_log.Log(LOG_ERROR, "BleBox: light not changed!");
					return false;
				}

				break;
			}
		}
	}
	if (output->ICMND.packettype == pTypeLimitlessLights && output->LIGHTING2.subtype == sTypeLimitlessRGBW)
	{
		std::string IPAddress = GetDeviceRevertIP(output);

		const _tLimitlessLights *pLed = reinterpret_cast<const _tLimitlessLights *>(pdata);
		int red, green, blue;
		float cHue = (360.0f / 255.0f)*float(pLed->value);//hue given was in range of 0-255
		hue2rgb(cHue, red, green, blue);

		char level[10];
		sprintf(level, "%02x%02x%02x%02x", red, green, blue, 255);
		std::string state(level);

		Json::Value root = SendCommand(IPAddress, "/s/" + state);
		if (root == "")
			return false;
	
		if (root["rgbw"].empty() == true)
		{
			_log.Log(LOG_ERROR, "BleBox: node 'rgbw' missing!");
			return false;
		}
		if (root["rgbw"]["desiredColor"].empty() == true)
		{
			_log.Log(LOG_ERROR, "BleBox: node 'desiredColor' missing!");
			return false;
		}

		if (root["rgbw"]["desiredColor"].asString() != state)
		{
			_log.Log(LOG_ERROR, "BleBox: rgbw not changed!");
			return false;
		}
	}
	
	return true;
}

void BleBox::SetSettings(const int pollIntervalSec)
{
	m_PollInterval = 30;

	if (pollIntervalSec > 1)
		m_PollInterval = pollIntervalSec;
}

void BleBox::Restart()
{
	StopHardware();
	StartHardware();
}

void BleBox::SendSwitch(const int NodeID, const int ChildID, const int BatteryLevel, const bool bOn, const double Level, const std::string &defaultname)
{ //TODO - remove this method, when in DomoticzHardware bug is fix (15 instead 16)
	double rlevel = (15.0 / 100.0)*Level;
	int level = int(rlevel);

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
		if (bOn && (nvalue == light2_sOn))
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
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), BatteryLevel);
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_BleBoxGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;

			root["status"] = "OK";
			root["title"] = "BleBoxGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,DeviceID,Unit FROM DeviceStatus WHERE (HardwareID=='%d')", pBaseHardware->m_HwdID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					BYTE id1, id2, id3, id4;
					char ip[20];
					sscanf(sd[2].c_str(), "%2hhx%2hhx%2hhx%2hhx", &id1, &id2, &id3, &id4);
					sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = ip;
					root["result"][ii]["Type"] = DevicesType[atoi(sd[3].c_str())].name;
					ii++;
				}
			}
		}


		void CWebServer::Cmd_BleBoxSetMode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
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
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox *pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxSetMode";

			int iMode1 = atoi(mode1.c_str());
			int iMode2 = atoi(mode2.c_str());

			m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == '%q')", iMode1, iMode2, hwid.c_str());
			pHardware->SetSettings(iMode1);
			pHardware->Restart();
		}


		void CWebServer::Cmd_BleBoxAddNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
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
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox *pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxAddNode";
			pHardware->AddNode(name, ip);
		}

		void CWebServer::Cmd_BleBoxUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = request::findValue(&req, "name");
			std::string ip = request::findValue(&req, "ip");
			if (
				(hwid == "") ||
				(nodeid == "") ||
				(name == "") ||
				(ip == "")
				)
				return;
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox *pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxUpdateNode";
			int ID = atoi(nodeid.c_str());
			pHardware->UpdateNode(ID, name, ip);
		}

		void CWebServer::Cmd_BleBoxRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if (
				(hwid == "") ||
				(nodeid == "")
				)
				return;
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox *pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxRemoveNode";
			int ID = atoi(nodeid.c_str());
			pHardware->RemoveNode(ID);
		}

		void CWebServer::Cmd_BleBoxClearNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				return;
			}

			std::string hwid = request::findValue(&req, "idx");
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardwareByIDType(hwid, HTYPE_BleBox);
			if (pBaseHardware == NULL)
				return;
			BleBox *pHardware = reinterpret_cast<BleBox*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "BleBoxClearNodes";
			pHardware->RemoveAllNodes();
		}

	}
}

Json::Value BleBox::SendCommand(const std::string &IPAddress, const std::string &command)
{
	std::vector<std::string> extraHeaders;
	std::string result;

	std::stringstream sstr;
	sstr << "http://" << IPAddress << command;

	std::string sURL = sstr.str();
	if (!HTTPClient::GET(sURL, extraHeaders, result))
	{
		_log.Log(LOG_ERROR, "BleBox: send '%s'command to %s failed!", command.c_str(), IPAddress.c_str());
		return "";
	}

	Json::Value root;
	Json::Reader jReader;
	if (!jReader.parse(result, root))
	{
		_log.Log(LOG_ERROR, "BleBox: Invalid json received!");
		return "";
	}

	if (root.size() == 0)
	{
		_log.Log(LOG_ERROR, "BleBox: Json is empty!");
		return "";
	}

	if (root.isArray())
		root = root[0];

	return root;
}

std::string BleBox::IdentifyDevice(const std::string &IPAddress)
{
	Json::Value root = SendCommand(IPAddress, "/api/device/state");
	if (root == "")
		return "";

	std::string result;

	if (root["device"].empty() == true)
	{
		if (root["type"].empty() == true)
		{
			_log.Log(LOG_ERROR, "BleBox: Invalid data received!");
			return "";
		}
		else
		{
			result = root["type"].asString();
		}
	}
	else
	{
		if (root["device"]["type"].empty() == true)
		{
			_log.Log(LOG_ERROR, "BleBox: Invalid device type received!");
			return "";
		}
		result = root["device"]["type"].asString();
	}
	return result;
}

void BleBox::AddNode(const std::string &name, const std::string &IPAddress)
{
	std::string deviceApiName = IdentifyDevice(IPAddress);
	if (deviceApiName.empty())
		return;

	int deviceTypeID = GetDeviceTypeByApiName(deviceApiName);
	if (deviceTypeID == -1)
		return;

	STR_DEVICE deviceType = DevicesType[deviceTypeID];

	std::string szIdx = IPToHex(IPAddress, deviceType.deviceID);

	m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d, '%q', %d, %d, %d, %d, 1, 12, 255, '%q', 0, 'Unavailable')",
		m_HwdID, szIdx.c_str(), deviceType.unit, deviceType.deviceID, deviceType.subType, deviceType.switchType, name.c_str());

	ReloadNodes();
}

bool BleBox::UpdateNode(const int id, const std::string &name, const std::string &IPAddress)
{
	std::string deviceApiName = IdentifyDevice(IPAddress);
	if (deviceApiName.empty())
		return false;

	int deviceTypeID = GetDeviceTypeByApiName(deviceApiName);
	if (deviceTypeID == -1)
		return false;

	STR_DEVICE deviceType = DevicesType[deviceTypeID];

	std::string szIdx = IPToHex(IPAddress, deviceType.deviceID);

	m_sql.safe_query("UPDATE DeviceStatus SET DeviceID='%q', Unit='%d', Type='%d', SubType='%d', SwitchType='%d', Name='%q' WHERE (HardwareID=='%d') AND (ID=='%d')", 
		szIdx.c_str(), deviceType.unit, deviceType.deviceID, deviceType.subType, deviceType.switchType, name.c_str(), m_HwdID, id);

	ReloadNodes();
	return true;
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
	boost::lock_guard<boost::mutex> l(m_mutex);

	m_devices.clear();
}

bool BleBox::LoadNodes()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,DeviceID, Unit FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			const std::vector<std::string> &sd = *itt;
			std::string addressIP = GetDeviceIP(sd[1]);
			int type = atoi(sd[2].c_str());
			m_devices.insert(std::pair<const std::string, const int>(addressIP, type));
		}
		return true;
	}
	else
	{
		_log.Log(LOG_ERROR, "BleBox: Cannot find any devices...");
		return false;
	}
}

void BleBox::ReloadNodes()
{
	UnloadNodes();
	LoadNodes();
}

