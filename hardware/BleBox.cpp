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

#define TOT_TYPE 2

const _STR_DEVICE DevicesType[TOT_TYPE] =
{
	{ 0, "switchBox", "Switch Box",int(pTypeLighting2), int(sTypeAC), int(STYPE_OnOff) },
	{ 1, "wLightBox", "Light Box", int(pTypeLimitlessLights), int(sTypeLimitlessRGBW), int(STYPE_Dimmer) }
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
	std::map<const std::string, const int>::const_iterator itt;
	for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
	{
		for (int i = 0; i < TOT_TYPE; ++i)
		{
			if (DevicesType[i].unit == itt->second)
			{
				Json::Value root = SendCommand(itt->first, "/api/relay/state");
				if (root == "")
					break;

				if (root["state"].empty() == true)
				{
					_log.Log(LOG_ERROR, "BleBox: node 'state' missing!");
					break;
				}

				int node = IPToUInt(itt->first);
				if (node != 0)
				{
					bool state = root["state"].asBool();

					SendSwitch(node, itt->second, 255, state, 0, DevicesType[i].name);
					SetHeartbeatReceived(); //TODO - another place
				}
			}
		}


	}
}

std::string BleBox::GetDeviceIP(const tRBUF *id)
{
	char ip[20];

	sprintf(ip, "%d.%d.%d.%d", id->LIGHTING2.id1, id->LIGHTING2.id2, id->LIGHTING2.id3, id->LIGHTING2.id4);
	return ip;

}

std::string BleBox::GetDeviceIP(const std::string &id)
{
	const char *pos = id.c_str();
	BYTE id1, id2, id3, id4;
	char ip[20];

	sscanf(pos, "%2hhx", &id1);
	pos += 2;
	sscanf(pos, "%2hhx", &id2);
	pos += 2;
	sscanf(pos, "%2hhx", &id3);
	pos += 2;
	sscanf(pos, "%2hhx", &id4);

	sprintf(ip, "%d.%d.%d.%d", id1, id2, id3, id4);
	return ip;
}


bool BleBox::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeLighting2 && output->LIGHTING2.subtype == sTypeAC)  // switch box
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

		std::string IPAddress = GetDeviceIP(output);

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

		return true;

	}

	return false;
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
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_BleBox)
				return;

			root["status"] = "OK";
			root["title"] = "BleBoxGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,DeviceID FROM DeviceStatus WHERE (HardwareID==%d)", iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["IP"] = sd[2];
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
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_BleBox)
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
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_BleBox)
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
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_BleBox)
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
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_BleBox)
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
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == NULL)
				return;
			if (pBaseHardware->HwdType != HTYPE_BleBox)
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

	if (root["device"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BleBox: Invalid data received!");
		return "";
	}
	if (root["device"]["type"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BleBox: Invalid device type received!");
		return "";
	}

	return root["device"]["type"].asString();
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

	std::vector<std::string> strarray;
	boost::split(strarray,IPAddress, boost::is_any_of("."));
	if (strarray.size() != 4)
		return;

	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", atoi(strarray[0].data()), atoi(strarray[1].data()), atoi(strarray[2].data()), atoi(strarray[3].data()));

	m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d, '%q', %d, %d, %d, %d, 1, 12, 255, '%q', 0, 'Unavailable')",
		m_HwdID, szIdx, deviceType.unit, deviceType.deviceID, deviceType.subType, deviceType.switchType, name.c_str());

	ReloadNodes();
}

bool BleBox::UpdateNode(const int id, const std::string &name, const std::string &IPAddress)
{
	std::vector<std::vector<std::string> > result;

	m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', DeviceID='%q' WHERE (HardwareID==%d) AND (ID=='%d')", 
		name.c_str(), IPAddress.c_str(), m_HwdID, id);

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
	boost::lock_guard<boost::mutex> l(m_mutex);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);

	UnloadNodes();
}

void BleBox::UnloadNodes()
{
	m_devices.clear();
}

bool BleBox::LoadNodes()
{
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

