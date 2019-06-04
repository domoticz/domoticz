#include "stdafx.h"
#ifdef WITH_OPENZWAVE
#include "OpenZWave.h"

#include <sstream>      // std::stringstream
#include <ctype.h>
#include <algorithm>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "hardwaretypes.h"

#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/mainworker.h"

#include "../json/json.h"
#include "../main/localtime_r.h"

//OpenZWave includes
#include "openzwave/Options.h"
#include "openzwave/Manager.h"
#include "openzwave/platform/Log.h"

#include "ZWaveCommands.h"

#include "serial/serial.h"

extern std::string szWWWFolder;

//Note!, Some devices uses the same instance for multiple values,
//to solve this we are going to use the Index value!, Except for COMMAND_CLASS_MULTI_INSTANCE

//Scale ID's
enum _eSensorScaleID
{
	SCALEID_UNUSED = 0,
	SCALEID_ENERGY,
	SCALEID_POWER,
	SCALEID_VOLTAGE,
	SCALEID_CURRENT,
	SCALEID_POWERFACTOR,
	SCALEID_GAS,
	SCALEID_CO2,
	SCALEID_WATER,
	SCALEID_MOISTRUE,
	SCALEID_TANK_CAPACITY,
	SCALEID_RAIN_RATE,
	SCALEID_SEISMIC_INTENSITY,
};

struct _tAlarmNameToIndexMapping
{
	std::string sLabel;
	uint8_t iIndex;
};

static const _tAlarmNameToIndexMapping AlarmToIndexMapping[] = {
	{ "General",			0x28 },
	{ "Smoke",				0x29 },
	{ "Carbon Monoxide",	0x2A },
	{ "Carbon Dioxide",		0x2B },
	{ "Heat",				0x2C },
	{ "Water",				0x2D },
	{ "Flood",				0x2D },
	{ "Alarm Level",		0x32 },
	{ "Alarm Type",			0x33 },
	{ "Access Control",		0x34 },
	{ "Burglar",			0x35 },
	{ "Home Security",		0x35 },
	{ "Power Management",	0x36 },
	{ "System",				0x37 },
	{ "Emergency",			0x38 },
	{ "Clock",				0x39 },
	{ "Appliance",			0x3A },
	{ "HomeHealth",			0x3B },
	{ "Siren",				0x3C },
	{ "Water Valve",		0x3D },
	{ "Weather",			0x3E },
	{ "Irrigation",			0x3F },
	{ "Gas",				0x40 },
	{ "", 0 }
};

uint8_t GetIndexFromAlarm(const std::string& sLabel)
{
	int ii = 0;
	while (AlarmToIndexMapping[ii].iIndex != 0)
	{
		if (sLabel.find(AlarmToIndexMapping[ii].sLabel) != std::string::npos)
			return AlarmToIndexMapping[ii].iIndex;
		ii++;
	}
	return 0;
}

enum _eAlarm_Types
{
	Alarm_General = 0,
	Alarm_Smoke,
	Alarm_CarbonMonoxide,
	Alarm_CarbonDioxide,
	Alarm_Heat,
	Alarm_Water,
	Alarm_Access_Control,
	Alarm_HomeSecurity,
	Alarm_Power_Management,
	Alarm_System,
	Alarm_Emergency,
	Alarm_Clock,
	Alarm_Appliance,
	Alarm_HomeHealth,
	Alarm_Siren,
	Alarm_Water_Valve,
	Alarm_Weather,
	Alarm_Irrigation,
	Alarm_Gas,
	Alarm_Count
};

static const _tAlarmNameToIndexMapping AlarmTypeToIndexMapping[] = {
	{ "General",			Alarm_General },
	{ "Smoke",				Alarm_Smoke },
	{ "Carbon Monoxide",	Alarm_CarbonMonoxide },
	{ "Carbon Dioxide",		Alarm_CarbonDioxide },
	{ "Heat",				Alarm_Heat },
	{ "Water",				Alarm_Water },
	{ "Access Control",		Alarm_Access_Control },
	{ "Home Security",		Alarm_HomeSecurity },
	{ "Power Management",	Alarm_Power_Management },
	{ "System",				Alarm_System },
	{ "Emergency",			Alarm_Emergency },
	{ "Clock",				Alarm_Clock },
	{ "Appliance",			Alarm_Appliance },
	{ "HomeHealth",			Alarm_HomeHealth },
	{ "Siren",				Alarm_Siren },
	{ "Water Valve",		Alarm_Water_Valve },
	{ "Weather",			Alarm_Weather },
	{ "Irrigation",			Alarm_Irrigation },
	{ "Gas",				Alarm_Gas },
	{ "", 0 }
};

uint8_t GetIndexFromAlarmType(const std::string& sLabel)
{
	int ii = 0;
	while (!AlarmTypeToIndexMapping[ii].sLabel.empty())
	{
		if (sLabel.find(AlarmTypeToIndexMapping[ii].sLabel) != std::string::npos)
			return AlarmTypeToIndexMapping[ii].iIndex;
		ii++;
	}
	return -1;
}

#pragma warning(disable: 4996)

extern std::string szStartupFolder;
extern std::string szUserDataFolder;

#define round(a) ( int ) ( a + .5 )

//Should be obsolete when OZW 2.0 comes out
uint16_t GetUInt16FromString(const std::string& inStr)
{
	uint16_t xID;
	std::stringstream ss;
	ss << std::hex << inStr;
	ss >> xID;
	return xID;
}

#ifdef _DEBUG
#define DEBUG_ZWAVE_INT
#endif

const char* cclassStr(uint8 cc)
{
	switch (cc) {
	default:
	case 0x00:
		return "NO OPERATION";
	case 0x20:
		return "BASIC";
	case 0x21:
		return "CONTROLLER REPLICATION";
	case 0x22:
		return "APPLICATION STATUS";
	case 0x23:
		return "ZIP SERVICES";
	case 0x24:
		return "ZIP SERVER";
	case 0x25:
		return "SWITCH BINARY";
	case 0x26:
		return "SWITCH MULTILEVEL";
	case 0x27:
		return "SWITCH ALL";
	case 0x28:
		return "SWITCH TOGGLE BINARY";
	case 0x29:
		return "SWITCH TOGGLE MULTILEVEL";
	case 0x2A:
		return "CHIMNEY FAN";
	case 0x2B:
		return "SCENE ACTIVATION";
	case 0x2C:
		return "SCENE ACTUATOR CONF";
	case 0x2D:
		return "SCENE CONTROLLER CONF";
	case 0x2E:
		return "ZIP CLIENT";
	case 0x2F:
		return "ZIP ADV SERVICES";
	case 0x30:
		return "SENSOR BINARY";
	case 0x31:
		return "SENSOR MULTILEVEL";
	case 0x32:
		return "METER";
	case 0x33:
		return "COLOR";
	case 0x34:
		return "ZIP ADV CLIENT";
	case 0x35:
		return "METER PULSE";
	case 0x38:
		return "THERMOSTAT HEATING";
	case 0x3C:
		return "METER_TBL_CONFIG";
	case 0x3D:
		return "METER_TBL_MONITOR";
	case 0x3E:
		return "METER_TBL_PUSH";
	case 0x40:
		return "THERMOSTAT MODE";
	case 0x42:
		return "THERMOSTAT OPERATING STATE";
	case 0x43:
		return "THERMOSTAT SETPOINT";
	case 0x44:
		return "THERMOSTAT FAN MODE";
	case 0x45:
		return "THERMOSTAT FAN STATE";
	case 0x46:
		return "CLIMATE CONTROL SCHEDULE";
	case 0x47:
		return "THERMOSTAT SETBACK";
	case 0x4C:
		return "DOOR LOCK LOGGING";
	case 0x4E:
		return "SCHEDULE ENTRY LOCK";
	case 0x50:
		return "BASIC WINDOW COVERING";
	case 0x51:
		return "MTP WINDOW COVERING";
	case 0x56:
		return "CRC16 ENCAP";
	case 0x5A:
		return "DEVICE RESET LOCALLY";
	case 0x5B:
		return "CENTRAL SCENE";
	case 0x5E:
		return "ZWAVE PLUS INFO";
	case 0x60:
		return "MULTI INSTANCE";
	case 0x62:
		return "DOOR LOCK";
	case 0x63:
		return "USER CODE";
	case 0x70:
		return "CONFIGURATION";
	case 0x71:
		return "ALARM";
	case 0x72:
		return "MANUFACTURER SPECIFIC";
	case 0x73:
		return "POWERLEVEL";
	case 0x75:
		return "PROTECTION";
	case 0x76:
		return "LOCK";
	case 0x77:
		return "NODE NAMING";
	case 0x7A:
		return "FIRMWARE UPDATE MD";
	case 0x7B:
		return "GROUPING NAME";
	case 0x7C:
		return "REMOTE ASSOCIATION ACTIVATE";
	case 0x7D:
		return "REMOTE ASSOCIATION";
	case 0x80:
		return "BATTERY";
	case 0x81:
		return "CLOCK";
	case 0x82:
		return "HAIL";
	case 0x84:
		return "WAKE UP";
	case 0x85:
		return "ASSOCIATION";
	case 0x86:
		return "VERSION";
	case 0x87:
		return "INDICATOR";
	case 0x88:
		return "PROPRIETARY";
	case 0x89:
		return "LANGUAGE";
	case 0x8A:
		return "TIME";
	case 0x8B:
		return "TIME PARAMETERS";
	case 0x8C:
		return "GEOGRAPHIC LOCATION";
	case 0x8D:
		return "COMPOSITE";
	case 0x8E:
		return "MULTI INSTANCE ASSOCIATION";
	case 0x8F:
		return "MULTI CMD";
	case 0x90:
		return "ENERGY PRODUCTION";
	case 0x91:
		return "MANUFACTURER PROPRIETARY";
	case 0x92:
		return "SCREEN MD";
	case 0x93:
		return "SCREEN ATTRIBUTES";
	case 0x94:
		return "SIMPLE AV CONTROL";
	case 0x95:
		return "AV CONTENT DIRECTORY MD";
	case 0x96:
		return "AV RENDERER STATUS";
	case 0x97:
		return "AV CONTENT SEARCH MD";
	case 0x98:
		return "SECURITY";
	case 0x99:
		return "AV TAGGING MD";
	case 0x9A:
		return "IP CONFIGURATION";
	case 0x9B:
		return "ASSOCIATION COMMAND CONFIGURATION";
	case 0x9C:
		return "SENSOR ALARM";
	case 0x9D:
		return "SILENCE ALARM";
	case 0x9E:
		return "SENSOR CONFIGURATION";
	case 0xEF:
		return "MARK";
	case 0xF0:
		return "NON INTEROPERABLE";
	}
	return "UNKNOWN";
}

COpenZWave::COpenZWave(const int ID, const std::string& devname) :
	m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_controllerID = 0;
	m_controllerNodeId = 0;
	m_bIsShuttingDown = false;
	m_initFailed = false;
	m_allNodesQueried = false;
	m_awakeNodesQueried = false;
	m_bInUserCodeEnrollmentMode = false;
	m_bNightlyNetworkHeal = false;
	m_pManager = NULL;
	m_bAeotecBlinkingMode = false;
}


COpenZWave::~COpenZWave(void)
{
	CloseSerialConnector();
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this home/node id
//-----------------------------------------------------------------------------
COpenZWave::NodeInfo* COpenZWave::GetNodeInfo(const unsigned int homeID, const int nodeID)
{
	for (std::list<NodeInfo>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		if ((it->homeId == homeID) && (it->nodeId == nodeID))
		{
			return &(*it);
		}
	}

	return NULL;
}

std::string COpenZWave::GetNodeStateString(const unsigned int homeID, const int nodeID)
{
	std::string strState = "Unknown";
	COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (!pNode)
		return strState;

	try
	{
		if (OpenZWave::Manager::Get()->IsNodeFailed(homeID, nodeID))
		{
			strState = "Dead";
		}
		else {
			if (OpenZWave::Manager::Get()->IsNodeAwake(homeID, nodeID))
				strState = "Awake";
			else
				strState = "Sleeping";
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return strState;
}

std::string COpenZWave::GetNodeGenericType(const bool bIsPlus, const uint32 homeID, const uint8 nodeID)
{
	if (bIsPlus)
		return m_pManager->GetNodeDeviceTypeString(homeID, nodeID);
	return m_pManager->GetNodeType(homeID, nodeID);
}

uint8_t COpenZWave::GetInstanceFromValueID(const OpenZWave::ValueID& vID)
{
	uint16_t instance;

	int commandClass = vID.GetCommandClassId();
	uint8_t vInstance = vID.GetInstance();//(See note on top of this file) GetInstance();
	uint16_t vIndex = vID.GetIndex();

	if (
		(commandClass == COMMAND_CLASS_MULTI_INSTANCE) ||
		(commandClass == COMMAND_CLASS_SENSOR_MULTILEVEL) ||
		(commandClass == COMMAND_CLASS_THERMOSTAT_SETPOINT) ||
		(commandClass == COMMAND_CLASS_SENSOR_BINARY) ||
		(commandClass == COMMAND_CLASS_CENTRAL_SCENE)
		)
	{
		instance = vIndex;
		//special case for sensor_multilevel
		if (commandClass == COMMAND_CLASS_SENSOR_MULTILEVEL)
		{
			uint16_t rIndex = instance;
			if (rIndex != vInstance)
			{
				if (rIndex == 1)
					instance = vInstance;
			}
		}
		else
		{
			if ((instance == 0) && (vInstance > 1))
				instance = vInstance;
		}
	}
	else
	{
		instance = vInstance;
		//if (commandClass == COMMAND_CLASS_SWITCH_MULTILEVEL)
		//{
		//	uint8_t rIndex = vInstance;
		//	if (rIndex != vIndex)
		//	{
		//		if (vInstance == 1)
		//		{
		//			instance = vIndex;
		//		}
		//	}
		//}
	}

	return (uint8_t)instance;
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification(OpenZWave::Notification const* _notification, void* _context)
{
	COpenZWave* pClass = static_cast<COpenZWave*>(_context);
	try
	{
		pClass->OnZWaveNotification(_notification);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	catch (std::exception& e)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception: %s!", e.what());
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Unknown Exception catched! %s:%d", std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::OnZWaveNotification(OpenZWave::Notification const* _notification)
{
	// Must do this inside a critical section to avoid conflicts with the main thread
	std::lock_guard<std::mutex> l(m_NotificationMutex);

	if (m_bIsShuttingDown)
		return;
	if (!m_pManager)
		return;

	//Send 2 OZW control panel
	m_ozwcp.OnCPNotification(_notification);

	if (m_bIsShuttingDown)
		return;
	m_updateTime = mytime(NULL);

	OpenZWave::ValueID vID = _notification->GetValueID();
	int commandClass = vID.GetCommandClassId();
	unsigned int _homeID = _notification->GetHomeId();
	uint8_t _nodeID = _notification->GetNodeId();

	uint8_t instance = GetInstanceFromValueID(vID);

	OpenZWave::Notification::NotificationType nType = _notification->GetType();
	switch (nType)
	{
	case OpenZWave::Notification::Type_DriverReady:
	{
		m_controllerID = _notification->GetHomeId();
		m_controllerNodeId = _notification->GetNodeId();
		_log.Log(LOG_STATUS, "OpenZWave: Driver Ready");
	}
	break;
	case OpenZWave::Notification::Type_NodeNew:
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		_log.Log(LOG_STATUS, "OpenZWave: New Node added. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_NodeAdded:
	{
		if ((_nodeID == 0) || (_nodeID == 255))
		{
			_log.Log(LOG_STATUS, "OpenZWave: Invalid NodeID received. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			return;
		}
		// Add the new node to our list
		NodeInfo nodeInfo;
		nodeInfo.homeId = _homeID;
		nodeInfo.nodeId = _nodeID;
		nodeInfo.polled = false;
		nodeInfo.HaveUserCodes = false;
		nodeInfo.IsPlus = m_pManager->IsNodeZWavePlus(_homeID, _nodeID);
		nodeInfo.Application_version = 0;
		nodeInfo.szType = m_pManager->GetNodeType(_homeID, _nodeID);
		nodeInfo.iVersion = m_pManager->GetNodeVersion(_homeID, _nodeID);
		nodeInfo.Manufacturer_name = m_pManager->GetNodeManufacturerName(_homeID, _nodeID);
		nodeInfo.Manufacturer_id = GetUInt16FromString(m_pManager->GetNodeManufacturerId(_homeID, _nodeID));
		nodeInfo.Product_type = GetUInt16FromString(m_pManager->GetNodeProductType(_homeID, _nodeID));
		nodeInfo.Product_id = GetUInt16FromString(m_pManager->GetNodeProductId(_homeID, _nodeID));
		nodeInfo.Product_name = m_pManager->GetNodeProductName(_homeID, _nodeID);
		nodeInfo.tClockDay = -1;
		nodeInfo.tClockHour = -1;
		nodeInfo.tClockMinute = -1;
		nodeInfo.tMode = -1;
		nodeInfo.tFanMode = -1;

		m_LastAlarmTypeReceived = -1;

		if ((_homeID == m_controllerID) && (_nodeID == m_controllerNodeId))
			nodeInfo.eState = NSTATE_AWAKE;	//controller is always awake
		else
			nodeInfo.eState = NTSATE_UNKNOWN;

		nodeInfo.LastSeen = m_updateTime;
		m_nodes.push_back(nodeInfo);
		m_LastIncludedNode = _nodeID;
		m_LastIncludedNodeType = nodeInfo.szType;
		m_bHaveLastIncludedNodeInfo = !nodeInfo.Product_name.empty();
		AddNode(_homeID, _nodeID, &nodeInfo);
		m_bControllerCommandInProgress = false;
	}
	break;
	case OpenZWave::Notification::Type_NodeRemoved:
	{
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		//A node has been removed from OpenZWave's list.  This may be due to a device being removed from the Z-Wave network, or because the application is closing
		_log.Log(LOG_STATUS, "OpenZWave: Node Removed. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		// Remove the node from our list
		for (std::list<NodeInfo>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
		{
			if ((it->homeId == _homeID) && (it->nodeId == _nodeID))
			{
				m_nodes.erase(it);
				//DeleteNode(_homeID, _nodeID);
				break;
			}
		}
		m_bControllerCommandInProgress = false;
		m_LastRemovedNode = _nodeID;
	}
	break;
	case OpenZWave::Notification::Type_NodeProtocolInfo:
		break;
	case OpenZWave::Notification::Type_DriverReset:
		m_nodes.clear();
		m_controllerID = _notification->GetHomeId();
		break;
	case OpenZWave::Notification::Type_ValueAdded:
		if ((_nodeID == 0) || (_nodeID == 255))
		{
			_log.Log(LOG_STATUS, "OpenZWave: Invalid NodeID received. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			return;
		}
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			// Add the new value to our list

			if (nodeInfo->Manufacturer_name.empty())
			{
				nodeInfo->IsPlus = m_pManager->IsNodeZWavePlus(_homeID, _nodeID);
				nodeInfo->Manufacturer_name = m_pManager->GetNodeManufacturerName(_homeID, _nodeID);
				nodeInfo->Manufacturer_id = GetUInt16FromString(m_pManager->GetNodeManufacturerId(_homeID, _nodeID));
				nodeInfo->Product_type = GetUInt16FromString(m_pManager->GetNodeProductType(_homeID, _nodeID));
				nodeInfo->Product_id = GetUInt16FromString(m_pManager->GetNodeProductId(_homeID, _nodeID));
			}

			nodeInfo->Instances[instance][commandClass].Values.push_back(vID);
			nodeInfo->LastSeen = m_updateTime;
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
			if (commandClass == COMMAND_CLASS_USER_CODE)
			{
				nodeInfo->HaveUserCodes = true;
			}
			AddValue(vID, nodeInfo);
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueAdded, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_SceneEvent:
		//Deprecated
		break;
	case OpenZWave::Notification::Type_ValueRemoved:
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			// Remove the value from out list
			for (std::list<OpenZWave::ValueID>::iterator it = nodeInfo->Instances[instance][commandClass].Values.begin(); it != nodeInfo->Instances[instance][commandClass].Values.end(); ++it)
			{
				if ((*it) == vID)
				{
					nodeInfo->Instances[instance][commandClass].Values.erase(it);
					nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
					nodeInfo->LastSeen = m_updateTime;
					break;
				}
			}
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueRemoved, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_ValueChanged:
		if ((_nodeID == 0) || (_nodeID == 255))
		{
			_log.Log(LOG_STATUS, "OpenZWave: Invalid NodeID received. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			return;
		}
		// One of the node values has changed
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->eState = NSTATE_AWAKE;
			nodeInfo->LastSeen = m_updateTime;
			UpdateValue(vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueChanged, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_ValueRefreshed:
		// One of the node values has changed
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->eState = NSTATE_AWAKE;
			//UpdateValue(vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueRefreshed, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_Notification:
	{
		NodeInfo* nodeInfo = GetNodeInfo(_homeID, _nodeID);
		if (nodeInfo == NULL)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Type_Notification, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			return;
		}
		uint8 subType = _notification->GetNotification();
		switch (subType)
		{
		case OpenZWave::Notification::Code_MsgComplete:
		{
			bool bWasDead = (nodeInfo->eState == NSTATE_DEAD);
			nodeInfo->eState = NSTATE_AWAKE;
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
			if (bWasDead)
				ForceUpdateForNodeDevices(m_controllerID, _nodeID);
		}
		break;
		case OpenZWave::Notification::Code_Awake:
		{
			bool bWasDead = (nodeInfo->eState == NSTATE_DEAD);
			nodeInfo->eState = NSTATE_AWAKE;
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
			if (bWasDead)
				ForceUpdateForNodeDevices(m_controllerID, _nodeID);
		}
		break;
		case OpenZWave::Notification::Code_Sleep:
		{
			bool bWasDead = (nodeInfo->eState == NSTATE_DEAD);
			nodeInfo->eState = NSTATE_SLEEP;
			if (bWasDead)
				ForceUpdateForNodeDevices(m_controllerID, _nodeID);
		}
		break;
		case OpenZWave::Notification::Code_Dead:
		{
			bool bWasDead = (nodeInfo->eState == NSTATE_DEAD);
			nodeInfo->eState = NSTATE_DEAD;
			if (!bWasDead)
				ForceUpdateForNodeDevices(m_controllerID, _nodeID);
		}
		_log.Log(LOG_STATUS, "OpenZWave: Received Node Dead notification from HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
		case OpenZWave::Notification::Code_Alive:
		{
			bool bWasDead = (nodeInfo->eState == NSTATE_DEAD);
			nodeInfo->eState = NSTATE_AWAKE;
			if (bWasDead)
				ForceUpdateForNodeDevices(m_controllerID, _nodeID);
		}
		break;
		case OpenZWave::Notification::Code_Timeout:
			//{
			//	nodeInfo->eState = NSTATE_DEAD;
			//	ForceUpdateForNodeDevices(m_controllerID, _nodeID);
			//}
			_log.Log(LOG_STATUS, "OpenZWave: Received timeout notification from HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			break;
		case OpenZWave::Notification::Code_NoOperation:
			//Code_NoOperation send to node
			break;
		default:
			_log.Log(LOG_STATUS, "OpenZWave: Received unknown notification type (%d) from HomeID: %u, NodeID: %d (0x%02x)", subType, _homeID, _nodeID, _nodeID);
			break;
		}
	}
	break;
	case OpenZWave::Notification::Type_Group:
		// One of the node's association groups has changed
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_Group, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_NodeEvent:
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		// We have received an event from the node, caused by a
		// basic_set or hail message.
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->eState = NSTATE_AWAKE;
			UpdateNodeEvent(vID, static_cast<int>(_notification->GetEvent()));
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
			nodeInfo->LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_NodeEvent, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_PollingDisabled:
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->polled = false;
			nodeInfo->LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_PollingDisabled, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_PollingEnabled:
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->polled = true;
			nodeInfo->LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_PollingEnabled, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_DriverFailed:
		m_initFailed = true;
		m_nodes.clear();
		_log.Log(LOG_ERROR, "OpenZWave: Driver Failed!!");
		break;
	case OpenZWave::Notification::Type_DriverRemoved:
		_log.Log(LOG_ERROR, "OpenZWave: Driver Removed!!");
		m_bIsShuttingDown = true;
		break;
	case OpenZWave::Notification::Type_AwakeNodesQueried:
		_log.Log(LOG_STATUS, "OpenZWave: Awake Nodes queried");
		m_awakeNodesQueried = true;
		break;
	case OpenZWave::Notification::Type_AllNodesQueried:
	case OpenZWave::Notification::Type_AllNodesQueriedSomeDead:
		m_awakeNodesQueried = true;
		m_allNodesQueried = true;
		if (nType == OpenZWave::Notification::Type_AllNodesQueried)
			_log.Log(LOG_STATUS, "OpenZWave: All Nodes queried");
		else
			_log.Log(LOG_STATUS, "OpenZWave: All Nodes queried (Some Dead)");
		break;
	case OpenZWave::Notification::Type_NodeQueriesComplete:
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		NodeQueried(_homeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_NodeNaming:
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			std::string product_name = m_pManager->GetNodeProductName(_homeID, _nodeID);
			if (nodeInfo->Product_name != product_name)
			{
				nodeInfo->IsPlus = m_pManager->IsNodeZWavePlus(_homeID, _nodeID);
				nodeInfo->Manufacturer_name = m_pManager->GetNodeManufacturerName(_homeID, _nodeID);
				nodeInfo->Manufacturer_id = GetUInt16FromString(m_pManager->GetNodeManufacturerId(_homeID, _nodeID));
				nodeInfo->Product_type = GetUInt16FromString(m_pManager->GetNodeProductType(_homeID, _nodeID));
				nodeInfo->Product_id = GetUInt16FromString(m_pManager->GetNodeProductId(_homeID, _nodeID));
				nodeInfo->Product_name = product_name;
				AddNode(_homeID, _nodeID, nodeInfo);
				m_bHaveLastIncludedNodeInfo = !product_name.empty();
			}
			nodeInfo->LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_NodeNaming, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_EssentialNodeQueriesComplete:
		//The queries on a node that are essential to its operation have been completed. The node can now handle incoming messages.
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		break;
	case OpenZWave::Notification::Type_ControllerCommand:
	{
#ifndef _DEBUG
		if (!m_bControllerCommandInProgress)
			return;
#endif
		_log.Log(LOG_STATUS, "OpenZWave: %s", _notification->GetAsString().c_str());
	}
	break;
	/*
		case OpenZWave::Notification::Type_UserAlerts:
		{
			OpenZWave::Notification::UserAlertNofification uacode = (OpenZWave::Notification::UserAlertNofification)_notification->GetUserAlertType();
			_log.Log(LOG_STATUS, "OpenZWave: %s", _notification->GetAsString().c_str());
		}
		break;
	*/
	case OpenZWave::Notification::Type_ManufacturerSpecificDBReady:
		_log.Log(LOG_STATUS, "OpenZWave: %s", _notification->GetAsString().c_str());
		break;
	default:
		_log.Log(LOG_STATUS, "OpenZWave: Received unhandled notification type (%d) from HomeID: %u, NodeID: %d (0x%02x)", nType, _homeID, _nodeID, _nodeID);
		_log.Log(LOG_STATUS, "OpenZWave: %s", _notification->GetAsString().c_str());
		break;
	}
}

void COpenZWave::StopHardwareIntern()
{
	//CloseSerialConnector();
}

void COpenZWave::EnableDisableDebug()
{
	int debugenabled = 0;
#ifdef _DEBUG
	debugenabled = 1;
#else
	m_sql.GetPreferencesVar("ZWaveEnableDebug", debugenabled);
#endif

	if (debugenabled)
	{
		OpenZWave::Options::Get()->AddOptionInt("SaveLogLevel", OpenZWave::LogLevel_Detail);
		OpenZWave::Options::Get()->AddOptionInt("QueueLogLevel", OpenZWave::LogLevel_Debug);
		OpenZWave::Options::Get()->AddOptionInt("DumpTriggerLevel", OpenZWave::LogLevel_Error);
	}
	else
	{
		OpenZWave::Options::Get()->AddOptionInt("SaveLogLevel", OpenZWave::LogLevel_Error);
		OpenZWave::Options::Get()->AddOptionInt("QueueLogLevel", OpenZWave::LogLevel_Error);
		OpenZWave::Options::Get()->AddOptionInt("DumpTriggerLevel", OpenZWave::LogLevel_Error);
	}
}

bool COpenZWave::OpenSerialConnector()
{
	m_allNodesQueried = false;
	m_updateTime = mytime(NULL);
	CloseSerialConnector();

	m_nodes.clear();
	std::string ConfigPath = szStartupFolder + "Config/";
	std::string UserPath = ConfigPath;
	if (szStartupFolder != szUserDataFolder)
	{
		UserPath = szUserDataFolder;
	}
	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL
	// the log file will appear in the program's working directory.
	_log.Log(LOG_STATUS, "OpenZWave: using config in: %s", ConfigPath.c_str());

	try
	{
		OpenZWave::Options::Create(ConfigPath, UserPath, "--SaveConfiguration=true ");
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		return false;
	}

	EnableDisableDebug();
	OpenZWave::Options::Get()->AddOptionInt("PollInterval", 60000); //enable polling each 60 seconds
	OpenZWave::Options::Get()->AddOptionBool("IntervalBetweenPolls", true);
	OpenZWave::Options::Get()->AddOptionBool("ValidateValueChanges", true);
	OpenZWave::Options::Get()->AddOptionBool("Associate", true);

	//Disable automatic config update for now! (Dev branch is behind master with configuration files)
	OpenZWave::Options::Get()->AddOptionBool("AutoUpdateConfigFile", false);

	//Set network key for security devices
	std::string sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
	m_sql.GetPreferencesVar("ZWaveNetworkKey", sValue);
	std::vector<std::string> splitresults;
	StringSplit(sValue, ",", splitresults);
	if (splitresults.size() != 16)
	{
		sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
		m_sql.UpdatePreferencesVar("ZWaveNetworkKey", sValue);
	}
	OpenZWave::Options::Get()->AddOptionString("NetworkKey", sValue, false);

	int nValue = 3000; //default 10000
	m_sql.GetPreferencesVar("ZWaveRetryTimeout", nValue);
	OpenZWave::Options::Get()->AddOptionInt("RetryTimeout", nValue);

	nValue = 0; //default true
	m_sql.GetPreferencesVar("ZWaveAssumeAwake", nValue);
	OpenZWave::Options::Get()->AddOptionBool("AssumeAwake", (nValue == 1) ? true : false);

	nValue = 1; //default true
	m_sql.GetPreferencesVar("ZWavePerformReturnRoutes", nValue);
	OpenZWave::Options::Get()->AddOptionBool("PerformReturnRoutes", (nValue == 1) ? true : false);

	try
	{
		OpenZWave::Options::Get()->Lock();

		OpenZWave::Manager::Create();
		m_pManager = OpenZWave::Manager::Get();
		if (m_pManager == NULL)
			return false;

		m_LastAlarmTypeReceived = -1;
		_log.Log(LOG_STATUS, "OpenZWave: Starting...");
		_log.Log(LOG_STATUS, "OpenZWave: Version: %s", GetVersionLong().c_str());

		m_bIsShuttingDown = false;
		// Add a callback handler to the manager.  The second argument is a context that
		// is passed to the OnNotification method.  If the OnNotification is a method of
		// a class, the context would usually be a pointer to that class object, to
		// avoid the need for the notification handler to be a static.
		m_pManager->AddWatcher(OnNotification, this);

		// Add a Z-Wave Driver
		// Modify this line to set the correct serial port for your PC interface.
#ifdef WIN32
		if (m_szSerialPort.size() < 4)
			return false;
		std::string szPort = m_szSerialPort.substr(3);
		int iPort = atoi(szPort.c_str());
		char szComm[15];
		if (iPort < 10)
			sprintf(szComm, "COM%d", iPort);
		else
			sprintf(szComm, "\\\\.\\COM%d", iPort);
		if (!m_pManager->AddDriver(szComm))
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unable to start with on port: %s", szComm);
			return false;
		}
#else
		if (!m_pManager->AddDriver(m_szSerialPort.c_str()))
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unable to start with on port: %s", m_szSerialPort.c_str());
			return false;
		}
#endif
		int nightly_heal = 0;
		m_sql.GetPreferencesVar("ZWaveEnableNightlyNetworkHeal", nightly_heal);
		m_bNightlyNetworkHeal = (nightly_heal != 0);

		//OpenZWave::Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

void COpenZWave::CloseSerialConnector()
{
	// program exit (clean up)
	m_bIsShuttingDown = true;
	std::lock_guard<std::mutex> l(m_NotificationMutex);

	if (!m_pManager)
		return;

	_log.Log(LOG_STATUS, "OpenZWave: Closed");

	try
	{
		OpenZWave::Manager::Get()->RemoveWatcher(OnNotification, this);
		OpenZWave::Manager::Destroy();
		OpenZWave::Options::Destroy();
	}
	catch (...)
	{
	}
	m_pManager = NULL;
}

bool COpenZWave::GetInitialDevices()
{
	m_controllerID = 0;
	m_controllerNodeId = 0;
	m_initFailed = false;
	m_awakeNodesQueried = false;
	m_allNodesQueried = false;
	m_bInUserCodeEnrollmentMode = false;

	//Connect and request initial devices
	OpenSerialConnector();
	return true;
}

bool COpenZWave::GetUpdates()
{
	return false;
}

bool COpenZWave::GetFailedState()
{
	return m_initFailed;
}

bool COpenZWave::GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID& nValue)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
		return false;

	for (std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[instanceID][commandClass].Values.begin(); itt != pNode->Instances[instanceID][commandClass].Values.end(); ++itt)
	{
		uint8_t cmdClass = itt->GetCommandClassId();
		if (cmdClass == commandClass)
		{
			nValue = *itt;
			return true;
		}
	}
	return false;
}

bool COpenZWave::GetValueByCommandClassIndex(const int nodeID, const int instanceID, const int commandClass, const uint16_t vIndex, OpenZWave::ValueID& nValue)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
		return false;

	for (std::list<OpenZWave::ValueID>::iterator ittValue = pNode->Instances[instanceID][commandClass].Values.begin(); ittValue != pNode->Instances[instanceID][commandClass].Values.end(); ++ittValue)
	{
		uint8_t cmdClass = ittValue->GetCommandClassId();
		if (cmdClass == commandClass)
		{
			try
			{
				if ((*ittValue).GetIndex() == vIndex)
				{
					nValue = *ittValue;
					return true;
				}
			}
			catch (OpenZWave::OZWException& ex)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
				return false;
			}
		}
	}
	return false;
}

bool COpenZWave::GetNodeConfigValueByIndex(const NodeInfo* pNode, const int index, OpenZWave::ValueID& nValue)
{
	for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance = pNode->Instances.begin(); ittInstance != pNode->Instances.end(); ++ittInstance)
	{
		for (std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds)
		{
			for (std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue != ittCmds->second.Values.end(); ++ittValue)
			{
				uint16_t vindex = ittValue->GetIndex();
				int vinstance = ittValue->GetInstance();
				uint8_t commandclass = ittValue->GetCommandClassId();
				if (
					(commandclass == COMMAND_CLASS_CONFIGURATION) &&
					(vindex == index)
					)
				{
					nValue = *ittValue;
					return true;
				}
				else if (
					(commandclass == COMMAND_CLASS_WAKE_UP) &&
					(vindex == index - 2000) //spacial case
					)
				{
					nValue = *ittValue;
					return true;
				}
				else if (
					(commandclass == COMMAND_CLASS_PROTECTION) &&
					(vinstance == index - 3000) //spacial case
					)
				{
					nValue = *ittValue;
					return true;
				}
			}
		}
	}
	return false;
}

bool COpenZWave::SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)
{
	if (m_pManager == NULL)
		return false;
	NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
	{
		if (!m_awakeNodesQueried)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Switch command not sent because not all Awake Nodes have been Queried!");
		}
		else {
			_log.Log(LOG_ERROR, "OpenZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
		}
		return false;
	}

	try
	{
		if (m_pManager->IsNodeFailed(m_controllerID, nodeID))
		{
			_log.Log(LOG_ERROR, "OpenZWave: Node has failed (or is not alive), Switch command not sent! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}

		_tZWaveDevice* pDevice = FindDevice(nodeID, instanceID, 0, ZWaveBase::ZDTYPE_SWITCH_DIMMER);
		if (pDevice)
		{
			if (
				((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0003)) ||
				((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0103)) ||
				((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0203))
				)
			{
				//Special case for the Aeotec Smart Switch
				if (commandClass == COMMAND_CLASS_SWITCH_MULTILEVEL)
				{
					pDevice = FindDevice(nodeID, instanceID, 0, COMMAND_CLASS_SWITCH_BINARY, ZWaveBase::ZDTYPE_SWITCH_NORMAL);
				}
			}
		}
		if (!pDevice)
		{
			//Try to find Binary type
			if ((value == 0) || (value == 255))
			{
				pDevice = FindDevice(nodeID, instanceID, 0, COMMAND_CLASS_SWITCH_BINARY, ZWaveBase::ZDTYPE_SWITCH_NORMAL);
			}
		}
		if (!pDevice)
			pDevice = FindDevice(nodeID, instanceID, 0);
		if (!pDevice)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Internal Node Device not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}

		bool bHandleAsBinary = false;
		if ((value == 0) || (value == 255))
		{
			if (pDevice->Manufacturer_id == 0x010f)
			{
				if (
					((pDevice->Product_id == 0x1000) && (pDevice->Product_type == 0x0203)) || //223
					((pDevice->Product_id == 0x3000) && (pDevice->Product_type == 0x0203)) || //223
					((pDevice->Product_id == 0x1000) && (pDevice->Product_type == 0x0403)) || //213
					((pDevice->Product_id == 0x2000) && (pDevice->Product_type == 0x0403))    //213
					)
				{
					//Special case for the Fibaro FGS213/223
					bHandleAsBinary = true;
				}
			}
			else if (pDevice->Manufacturer_id == 0x0154)
			{
				if ((pDevice->Product_id == 0x0003) && (pDevice->Product_type == 0x0005))
				{
					//Special case for the Popp 009501 Flow Stop
					bHandleAsBinary = true;
				}
			}
		}

		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		uint8_t svalue = (uint8_t)value;

		if ((pDevice->devType == ZWaveBase::ZDTYPE_SWITCH_NORMAL) || (bHandleAsBinary))
		{
			//On/Off device
			bool bFound = (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_SWITCH_BINARY, vID) == true);
			if (!bFound)
				bFound = (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_DOOR_LOCK, vID) == true);
			if (!bFound)
				bFound = (GetValueByCommandClassIndex(nodeID, instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL, ValueID_Index_SwitchMultiLevel::Level, vID) == true);
			if (bFound)
			{
				OpenZWave::ValueID::ValueType vType = vID.GetType();
				_log.Log(LOG_NORM, "OpenZWave: Domoticz has send a Switch command! NodeID: %d (0x%02x)", nodeID, nodeID);
				if (vType == OpenZWave::ValueID::ValueType_Bool)
				{
					if (svalue == 0) {
						//Off
						m_pManager->SetValue(vID, false);
						pDevice->intvalue = 0;
					}
					else {
						//On
						m_pManager->SetValue(vID, true);
						pDevice->intvalue = 255;
					}
				}
				else if (vType == OpenZWave::ValueID::ValueType_Byte)
				{
					if (svalue == 0) {
						//Off
						m_pManager->SetValue(vID, 0);
						pDevice->intvalue = 0;
					}
					else {
						//On
						m_pManager->SetValue(vID, 255);
						pDevice->intvalue = 255;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
					return false;
				}

				if (svalue == 0)
				{
					//Device is switched off, lets see if there is a power sensor for this device,
					//and set its value to 0 as well
					_tZWaveDevice* pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
					if (pPowerDevice == NULL)
					{
						pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWER);
					}
					if (pPowerDevice != NULL)
					{
						pPowerDevice->floatValue = 0;
						SendDevice2Domoticz(pPowerDevice);
					}
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! NodeID: %d (0x%02x), instanceID: %d", nodeID, nodeID, instanceID);
				return false;
			}
		}
		else
		{
			//dimmable light  device
			if (GetValueByCommandClassIndex(nodeID, instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL, ValueID_Index_SwitchMultiLevel::Level, vID) == true)
			{
				if ((svalue > 99) && (svalue != 255))
					svalue = 99;
				pDevice->intvalue = svalue;
				_log.Log(LOG_NORM, "OpenZWave: Domoticz has send a Switch command!, Level: %d, NodeID: %d (0x%02x)", svalue, nodeID, nodeID);

				if (vID.GetType() == OpenZWave::ValueID::ValueType_Byte)
				{
					if (!m_pManager->SetValue(vID, svalue))
					{
						_log.Log(LOG_ERROR, "OpenZWave: Error setting Switch Value! NodeID: %d (0x%02x)", nodeID, nodeID);
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: SwitchLight, Unhandled value type: %d, %s:%d", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
					return false;
				}

				if (svalue == 0)
				{
					//Device is switched off, lets see if there is a power sensor for this device,
					//and set its value to 0 as well
					_tZWaveDevice* pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
					if (pPowerDevice == NULL)
					{
						pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, pDevice->indexID, ZDTYPE_SENSOR_POWER);
					}
					if (pPowerDevice != NULL)
					{
						pPowerDevice->floatValue = 0;
						SendDevice2Domoticz(pPowerDevice);
					}
				}

			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
				return false;
			}
		}
		m_updateTime = mytime(NULL);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	catch (...)
	{
	}
	return false;
}

bool COpenZWave::HasNodeFailed(const int nodeID)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
		if (!pNode)
			return true;
		return m_pManager->IsNodeFailed(m_controllerID, nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::SwitchColor(const int nodeID, const int instanceID, const int commandClass, const std::string& ColorStr)
{
	if (m_pManager == NULL)
		return false;

	try
	{
		NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
		if (!pNode)
		{
			if (!m_awakeNodesQueried)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Switch command not sent because not all Awake Nodes have been Queried!");
			}
			else {
				_log.Log(LOG_ERROR, "OpenZWave: Node not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			}
			return false;
		}
		if (m_pManager->IsNodeFailed(m_controllerID, nodeID))
		{
			_log.Log(LOG_ERROR, "OpenZWave: Node has failed (or is not alive), Switch command not sent! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}

		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClassIndex(nodeID, instanceID, COMMAND_CLASS_COLOR_CONTROL, ValueID_Index_Color::Color, vID) == true)
		{
			std::string OutColorStr = ColorStr;
			if (pNode->Manufacturer_id == 0x0131)
			{
				if ((pNode->Product_type == 0x0002) && (pNode->Product_id == 0x0002))
				{
					if (pNode->Application_version < 106)
					{
						if (OutColorStr.size() > 9)
						{
							//Old Zipato RGB bulp firmware does not support cold white
							OutColorStr = OutColorStr.substr(0, 9);
						}
					}
				}
				if ((pNode->Product_type == 0x0002) && (pNode->Product_id == 0x0003))
				{
					if (pNode->Application_version < 106)
					{
						if (OutColorStr.size() == 11)
						{
							//Zipato Bulb2 does not support cold white and warm white at the same time
							std::stringstream sstr;
							std::string RGB = OutColorStr.substr(0, 7);
							unsigned wWhite = strtoul(OutColorStr.substr(7, 2).c_str(), NULL, 16);
							unsigned cWhite = strtoul(OutColorStr.substr(9, 2).c_str(), NULL, 16);
							if (wWhite > cWhite)
							{
								wWhite = wWhite + cWhite;
								cWhite = 0;
							}
							else
							{
								cWhite = wWhite + cWhite;
								wWhite = 0;
							}
							sstr << RGB
								<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << wWhite
								<< std::setw(2) << std::uppercase << std::hex << std::setfill('0') << std::hex << cWhite;

							OutColorStr = sstr.str();
							// TODO: remove this print once Zipato Bulb 2 workaround has been verified
							// Gizmocuz: This has been in for quite some while, is it verified ?
							//_log.Debug(DEBUG_NORM, "OpenZWave::SwitchColor Workaround for Zipato Bulb 2 ColorStr: '%s', OutColorStr: '%s'",
							  //       ColorStr.c_str(), OutColorStr.c_str());
						}
					}
				}
			}

			OpenZWave::ValueID::ValueType vType = vID.GetType();
			if (vType == OpenZWave::ValueID::ValueType_String)
			{
				if (!m_pManager->SetValue(vID, OutColorStr))
				{
					_log.Log(LOG_ERROR, "OpenZWave: Error setting Switch Value! NodeID: %d (0x%02x)", nodeID, nodeID);
					return false;
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: SwitchColor, Unhandled value type: %d, %s:%d", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return false;
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}
		m_updateTime = mytime(NULL);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	catch (...)
	{

	}
	return false;
}

void COpenZWave::SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value)
{
	if (m_pManager == NULL)
		return;
	OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
	if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_SETPOINT, vID) == true)
	{
		std::string vUnits = m_pManager->GetValueUnits(vID);
		float temp = value;
		if (vUnits == "F")
		{
			//Convert to Fahrenheit
			temp = (float)ConvertToFahrenheit(temp);
		}
		try
		{
			if (vID.GetType() == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (!m_pManager->SetValue(vID, temp))
				{
					_log.Log(LOG_ERROR, "OpenZWave: Error setting Thermostat Setpoint Value! NodeID: %d (0x%02x)", nodeID, nodeID);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
			m_updateTime = mytime(NULL);
		}
		catch (OpenZWave::OZWException& ex)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		}
	}
}

void COpenZWave::AddValue(const OpenZWave::ValueID& vID, const NodeInfo* pNodeInfo)
{
	if (m_pManager == NULL)
		return;
	if (m_controllerID == 0)
		return;
	uint8_t commandclass = vID.GetCommandClassId();

	//Ignore some command classes, values are already added before calling this function
	if (
		(commandclass == COMMAND_CLASS_BASIC) ||
		(commandclass == COMMAND_CLASS_SWITCH_ALL) ||
		(commandclass == COMMAND_CLASS_POWERLEVEL)
		)
		return;

	unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();

	uint8_t vInstance = vID.GetInstance();//(See note on top of this file) GetInstance();
	uint16_t vIndex = vID.GetIndex();

	uint8_t vOrgInstance = vInstance;
	uint16_t vOrgIndex = vIndex;

	OpenZWave::ValueID::ValueType vType = vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre = vID.GetGenre();
	std::string vLabel = m_pManager->GetValueLabel(vID);

	if (commandclass == COMMAND_CLASS_CONFIGURATION)
	{
		std::string vUnits = m_pManager->GetValueUnits(vID);
#ifdef _DEBUG
		_log.Log(LOG_STATUS, "OpenZWave: Value_Added: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d, Id: 0x%llX", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex, vID.GetId());
#else
		_log.Debug(DEBUG_HARDWARE, "OpenZWave: Value_Added: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d, Id: 0x%llX", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vInstance, vIndex, vID.GetId());
#endif
		return;
	}

	if (commandclass == COMMAND_CLASS_VERSION)
	{
		if (vOrgIndex == ValueID_Index_Version::Application)
		{
			COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
			if (pNode)
			{
				if (vType == OpenZWave::ValueID::ValueType_String)
				{
					std::string strValue;
					if (m_pManager->GetValueAsString(vID, &strValue) == true)
					{
						pNode->Application_version = (int)(atof(strValue.c_str()) * 100);
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
					return;
				}
			}
		}
	}

	//Ignore non-user types
	if (vGenre != OpenZWave::ValueID::ValueGenre_User)
		return;

	uint8_t instance = GetInstanceFromValueID(vID);

	std::string vUnits = m_pManager->GetValueUnits(vID);
	_log.Log(LOG_NORM, "OpenZWave: Value_Added: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex);

	if ((instance == 0) && (NodeID == m_controllerID))
		return;// We skip instance 0 if there are more, since it should be mapped to other instances or their superposition

	_tZWaveDevice _device;
	_device.nodeID = NodeID;
	_device.commandClassID = commandclass;
	_device.scaleID = -1;
	_device.instanceID = instance;
	_device.indexID = 0;
	_device.hasWakeup = m_pManager->IsNodeAwake(HomeID, NodeID);
	_device.isListening = m_pManager->IsNodeListeningDevice(HomeID, NodeID);

	_device.Manufacturer_id = pNodeInfo->Manufacturer_id;
	_device.Product_id = pNodeInfo->Product_id;
	_device.Product_type = pNodeInfo->Product_type;

	if (!vLabel.empty())
		_device.label = vLabel;

	float fValue = 0;
	int iValue = 0;
	bool bValue = false;
	uint8_t byteValue = 0;
	int32 intValue = 0;

	// We choose SwitchMultilevel first, if not available, SwhichBinary is chosen
	if (
		(commandclass == COMMAND_CLASS_SWITCH_BINARY) ||
		(commandclass == COMMAND_CLASS_SENSOR_BINARY)
		)
	{
		_device.devType = ZDTYPE_SWITCH_NORMAL;
		if (vType == OpenZWave::ValueID::ValueType_Bool)
		{
			if (m_pManager->GetValueAsBool(vID, &bValue) == true)
			{
				if (bValue == true)
					_device.intvalue = 255;
				else
					_device.intvalue = 0;
			}
		}
		else if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (byteValue == 0)
				_device.intvalue = 0;
			else
				_device.intvalue = 255;
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
			return;
		}
		InsertDevice(_device);
	}
	else if (commandclass == COMMAND_CLASS_SWITCH_MULTILEVEL)
	{
		if (vOrgIndex == ValueID_Index_SwitchMultiLevel::Level)
		{
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				if (m_pManager->GetValueAsByte(vID, &byteValue) == true)
				{
					_device.devType = ZDTYPE_SWITCH_DIMMER;
					_device.intvalue = byteValue;
					InsertDevice(_device);
					if (instance == 1)
					{
						if (IsNodeRGBW(HomeID, NodeID))
						{
							_device.label = "RGBW";
							_device.devType = ZDTYPE_SWITCH_RGBW;
							_device.instanceID = 100;
							InsertDevice(_device);
						}
					}
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_COLOR_CONTROL)
	{
		if (vOrgIndex == ValueID_Index_Color::Color)
		{
			if (vType == OpenZWave::ValueID::ValueType_String)
			{
				_device.intvalue = 0;
				InsertDevice(_device);
				_device.label = "RGBW";
				_device.devType = ZDTYPE_SWITCH_COLOR;
				_device.instanceID = 101;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_USER_CODE)
	{
		/*
		//We already stored our codes in the nodeinfo
		if (vLabel.find("Code ")==0)
		{
		if ((vType == OpenZWave::ValueID::ValueType_Raw) || (vType == OpenZWave::ValueID::ValueType_String))
		{
		std::string strValue;
		if (m_pManager->GetValueAsString(vID, &strValue) == true)
		{
		while (1==0);
		}
		}
		}
		*/
	}
	else if (commandclass == COMMAND_CLASS_BASIC_WINDOW_COVERING)
	{
		_device.devType = ZDTYPE_SWITCH_NORMAL;
		if (vOrgIndex == ValueID_Index_BasicWindowCovering::Open)
		{
			_device.intvalue = 255;
		}
		else if (vOrgIndex == ValueID_Index_BasicWindowCovering::Close)
		{
			_device.intvalue = 0;
		}
		InsertDevice(_device);
	}
	else if ((commandclass == COMMAND_CLASS_ALARM) || (commandclass == COMMAND_CLASS_SENSOR_ALARM))
	{
		int newInstance = GetIndexFromAlarm(vLabel);
		if (newInstance != 0)
		{
			_device.instanceID = newInstance;

			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				if (m_pManager->GetValueAsByte(vID, &byteValue) == true)
				{
					//1.4
					_device.devType = ZDTYPE_SWITCH_NORMAL;
					if (byteValue == 0)
						_device.intvalue = 0;
					else
						_device.intvalue = 255;
					InsertDevice(_device);
				}
			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				//1.6
				//std::vector<std::string> tModes;
				//m_pManager->GetValueListItems(vID, &tModes);

				int32 listValue = 0;
				if (m_pManager->GetValueListSelection(vID, &listValue))
				{
					_device.devType = ZDTYPE_SWITCH_NORMAL;
					if (listValue == 0) //clear
						_device.intvalue = 0;
					else
						_device.intvalue = 255;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else
		{
			if (
				(vLabel.find("SourceNodeId") == std::string::npos) &&
				(vLabel.find("Previous Event Cleared") == std::string::npos)
				)
			{
				_log.Log(LOG_STATUS, "OpenZWave: Value_Added: Unhandled Label: %s, Unit: %s", vLabel.c_str(), vUnits.c_str());
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_METER)
	{
		if (
			(vLabel.find("Exporting") != std::string::npos) ||
			(vLabel.find("Interval") != std::string::npos) ||
			(vLabel.find("Previous Reading") != std::string::npos)
			)
			return; //Not interested

		//Meter device
		if (
			(vLabel.find("Energy") != std::string::npos) ||
			(vLabel.find("Power") != std::string::npos)
			)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					if (vLabel.find("Energy") != std::string::npos)
						_device.scaleID = SCALEID_ENERGY;
					else
						_device.scaleID = SCALEID_POWER;
					_device.scaleMultiply = 1;
					if ((vUnits == "kWh") || (vUnits == "kVAh"))
					{
						_device.scaleMultiply = 1000;
						_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
					}
					else
					{
						_device.devType = ZDTYPE_SENSOR_POWER;
					}
					_device.floatValue = fValue * _device.scaleMultiply;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Voltage") != std::string::npos)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_VOLTAGE;
					_device.devType = ZDTYPE_SENSOR_VOLTAGE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Current") != std::string::npos)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_CURRENT;
					_device.devType = ZDTYPE_SENSOR_AMPERE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Power Factor") != std::string::npos)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_POWERFACTOR;
					_device.devType = ZDTYPE_SENSOR_PERCENTAGE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Gas") != std::string::npos)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_GAS;
					_device.devType = ZDTYPE_SENSOR_GAS;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Water") != std::string::npos)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_WATER;
					_device.devType = ZDTYPE_SENSOR_WATER;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Unknown") != std::string::npos)
		{
			//It's unknown, so don't handle it
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled Meter type: %s, class: 0x%02X (%s), NodeID: %d (0x%02x), Index: %d, Instance: %d", vLabel.c_str(), commandclass, cclassStr(commandclass), NodeID, NodeID, vOrgIndex, vOrgInstance);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_SENSOR_MULTILEVEL)
	{
		if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::WaterTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::SoilTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::TargetTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::BoilerWaterTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::DHWTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::OutsideTemperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::ExhaustTemperature)
			) {
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					if (vUnits == "F")
					{
						//Convert to celcius
						fValue = float((fValue - 32) * (5.0 / 9.0));
					}
					_device.floatValue = fValue;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_TEMPERATURE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Luminance)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					if (vUnits != "lux")
					{
						//convert from % to Lux (where max is 1000 Lux)
						fValue = (1000.0f / 100.0f) * fValue;
						if (fValue > 1000.0f)
							fValue = 1000.0f;
					}

					_device.floatValue = fValue;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_LIGHT;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::RelativeHumidity)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::SoilHumidity)
			) {
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.intvalue = round(fValue);
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_HUMIDITY;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Ultraviolet)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_UV;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Velocity)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_VELOCITY;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::BarometricPressure)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::AtmosphericPressure)
			)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue * 10.0f;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_BAROMETER;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::DewPoint)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.commandClassID = 49;
					_device.devType = ZDTYPE_SENSOR_DEWPOINT;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Power)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.scaleID = SCALEID_POWER;
					_device.scaleMultiply = 1;
					if ((vUnits == "kWh") || (vUnits == "kVAh"))
					{
						_device.scaleMultiply = 1000;
						_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
					}
					else
					{
						_device.devType = ZDTYPE_SENSOR_POWER;
					}
					_device.floatValue = fValue * _device.scaleMultiply;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Voltage)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_VOLTAGE;
					_device.devType = ZDTYPE_SENSOR_VOLTAGE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Current)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_CURRENT;
					_device.devType = ZDTYPE_SENSOR_AMPERE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vLabel.find("Water") != std::string::npos) //water flow ?
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_WATER;
					_device.devType = ZDTYPE_SENSOR_WATER;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::CO2)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_CO2;
					_device.devType = ZDTYPE_SENSOR_CO2;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Moisture)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_MOISTRUE;
					_device.devType = ZDTYPE_SENSOR_MOISTURE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::TankCapacity)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_TANK_CAPACITY;
					_device.devType = ZDTYPE_SENSOR_TANK_CAPACITY;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::General)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.devType = ZDTYPE_SWITCH_NORMAL;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::RainRate)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_RAIN_RATE;
					_device.devType = ZDTYPE_SENSOR_CUSTOM;
					_device.custom_label = "mm/h";
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::SeismicIntensity)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
				{
					_device.floatValue = fValue;
					_device.scaleMultiply = 1;
					_device.scaleID = SCALEID_SEISMIC_INTENSITY;
					_device.devType = ZDTYPE_SENSOR_PERCENTAGE;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else
		{
			_log.Log(LOG_STATUS, "OpenZWave: Value_Added: Unhandled Label: %s, Unit: %s, Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", vLabel.c_str(), vUnits.c_str(), static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex);
		}
	}
	else if (commandclass == COMMAND_CLASS_BATTERY)
	{
		if (_device.isListening)
		{
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				UpdateDeviceBatteryStatus(NodeID, byteValue);
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_SETPOINT)
	{
		if (vType == OpenZWave::ValueID::ValueType_Decimal)
		{
			if (m_pManager->GetValueAsFloat(vID, &fValue) == true)
			{
				if (vUnits == "F")
				{
					//Convert to celcius
					fValue = float((fValue - 32) * (5.0 / 9.0));
				}
				_device.floatValue = fValue;
				_device.commandClassID = COMMAND_CLASS_THERMOSTAT_SETPOINT;
				_device.devType = ZDTYPE_SENSOR_SETPOINT;
				InsertDevice(_device);
				SendDevice2Domoticz(&_device);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_MODE)
	{
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (!pNode)
			return;
		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			if (vOrgIndex == ValueID_Index_ThermostatMode::Mode)
			{
				pNode->tModes.clear();
				m_pManager->GetValueListItems(vID, &pNode->tModes);
				if (!pNode->tModes.empty())
				{
					try
					{
						std::string vListStr;
						if (m_pManager->GetValueListSelection(vID, &vListStr))
						{
							int32 vMode = Lookup_ZWave_Thermostat_Modes(pNode->tModes, vListStr);
							if (vMode != -1)
							{
								pNode->tMode = vMode;
								_device.intvalue = vMode;
								_device.commandClassID = COMMAND_CLASS_THERMOSTAT_MODE;
								_device.devType = ZDTYPE_SENSOR_THERMOSTAT_MODE;
								InsertDevice(_device);
								SendDevice2Domoticz(&_device);
							}
						}
					}
					catch (...)
					{

					}
				}
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_MODE)
	{
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (!pNode)
			return;
		if (vOrgIndex == ValueID_Index_ThermostatFanMode::FanMode)
		{
			if (vType == OpenZWave::ValueID::ValueType_List)
			{
				pNode->tFanModes.clear();
				m_pManager->GetValueListItems(vID, &pNode->tFanModes);
				if (!pNode->tFanModes.empty())
				{
					try
					{
						std::string vListStr;
						if (m_pManager->GetValueListSelection(vID, &vListStr))
						{
							int32 vMode = Lookup_ZWave_Thermostat_Fan_Modes(vListStr);
							if (vMode != -1)
							{
								pNode->tFanMode = vMode;
								_device.intvalue = vMode;
								_device.commandClassID = COMMAND_CLASS_THERMOSTAT_FAN_MODE;
								_device.devType = ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE;
								InsertDevice(_device);
								SendDevice2Domoticz(&_device);
							}
						}
					}
					catch (...)
					{

					}
				}
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_CLOCK)
	{
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (!pNode)
			return;

		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			if (vOrgIndex == ValueID_Index_Clock::Day)
			{
				int32 vDay;
				try
				{
					if (m_pManager->GetValueListSelection(vID, &vDay))
					{
						if (vDay > 0)
						{
							pNode->tClockDay = vDay - 1;
						}
					}
				}
				catch (...)
				{

				}
			}
		}
		else if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
				return;
			if (vOrgIndex == ValueID_Index_Clock::Hour)
			{
				pNode->tClockHour = byteValue;
			}
			else if (vOrgIndex == ValueID_Index_Clock::Minute)
			{
				pNode->tClockMinute = byteValue;
				if (
					(pNode->tClockDay != -1) &&
					(pNode->tClockHour != -1) &&
					(pNode->tClockMinute != -1)
					)
				{
					_log.Log(LOG_NORM, "OpenZWave: NodeID: %d (0x%02x), Thermostat Clock: %s %02d:%02d", NodeID, NodeID, ZWave_Clock_Days(pNode->tClockDay), pNode->tClockHour, pNode->tClockMinute);
					_device.intvalue = (pNode->tClockDay * (24 * 60)) + (pNode->tClockHour * 60) + pNode->tClockMinute;
					_device.commandClassID = COMMAND_CLASS_CLOCK;
					_device.devType = ZDTYPE_SENSOR_THERMOSTAT_CLOCK;
					InsertDevice(_device);
					SendDevice2Domoticz(&_device);
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE)
	{
		if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
				return;
		}
		if (vType == OpenZWave::ValueID::ValueType_Schedule)
		{
			//we make our own schedule
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
			return;
		}
		//Ignore this class, we can make our own schedule with timers
		//_log.Log(LOG_STATUS, "OpenZWave: Unhandled class: 0x%02X (%s), NodeID: %d (0x%02x), Index: %d, Instance: %d", commandclass, cclassStr(commandclass), NodeID, NodeID, vOrgIndex, vOrgInstance);
	}
	else if (commandclass == COMMAND_CLASS_CENTRAL_SCENE)
	{
		if (vOrgIndex == ValueID_Index_CentralScene::SceneCount)
		{
			if (vType == OpenZWave::ValueID::ValueType_Int)
			{
				if (m_pManager->GetValueAsInt(vID, &iValue) == true)
				{
					while (1 == 0);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
		else if ((vOrgIndex >= ValueID_Index_CentralScene::Scene_1) && (vOrgIndex <= ValueID_Index_CentralScene::Scene_255))
		{
			int32 lValue = 0;
			if (vType == OpenZWave::ValueID::ValueType_List)
			{
				if (m_pManager->GetValueListSelection(vID, &lValue) == false)
					return;
			}
			if ((lValue == 0) || (lValue == 2)) // CentralSceneMask_KeyReleased
				return;
			//if (lValue != 1)
				//return; //only accept CentralSceneMask_KeyPressed1time

			_device.instanceID = (uint8_t)vIndex;
			_device.devType = ZDTYPE_CENTRAL_SCENE;
			_device.intvalue = lValue;
			InsertDevice(_device);
		}
	}
	else if (commandclass == COMMAND_CLASS_DOOR_LOCK)
	{
		if (
			(vLabel.find("Locked") != std::string::npos) ||
			(vLabel.find("Unlocked") != std::string::npos)
			)
		{
			_device.devType = ZDTYPE_SWITCH_NORMAL;
			if (vType == OpenZWave::ValueID::ValueType_Bool)
			{
				if (m_pManager->GetValueAsBool(vID, &bValue) == true)
				{
					if (bValue == true)
						_device.intvalue = 255;
					else
						_device.intvalue = 0;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_SCENE_ACTIVATION)
	{
		if (vOrgIndex != ValueID_Index_SceneActivation::SceneID)
			return;
		if (vType != OpenZWave::ValueID::ValueType_Int)
			return; //not interested in you
		if (m_pManager->GetValueAsInt(vID, &intValue) == false)
			return;
		_device.instanceID = instance;
		_device.devType = ZDTYPE_CENTRAL_SCENE;
		_device.intvalue = intValue;
		InsertDevice(_device);
	}
	else
	{
		//Unhandled
		_log.Log(LOG_STATUS, "OpenZWave: Unhandled class: 0x%02X (%s), NodeID: %d (0x%02x), Index: %d, Instance: %d", commandclass, cclassStr(commandclass), NodeID, NodeID, vOrgIndex, vOrgInstance);
		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			//std::vector<std::string > vStringList;
			//if (m_pManager->GetValueListItems(vID,&vStringList)==true)
			//{
			//}
		}
	}
}

void COpenZWave::UpdateNodeEvent(const OpenZWave::ValueID& vID, int EventID)
{
	if (m_pManager == NULL)
		return;
	if (m_controllerID == 0)
		return;

	//if (m_nodesQueried==false)
	//return; //only allow updates when node Query is done

	unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();
	uint16_t instance = vID.GetInstance();
	if (instance == 0)
		return;
	uint16_t index = vID.GetIndex();
	instance = index;

	uint8_t commandclass = vID.GetCommandClassId();

	if ((commandclass == COMMAND_CLASS_ALARM) || (commandclass == COMMAND_CLASS_SENSOR_ALARM))
	{
		std::string vLabel = "";
		if (commandclass != 0)
		{
			m_pManager->GetValueLabel(vID);
		}
		instance = GetIndexFromAlarm(vLabel);
		if (instance == 0)
			return;
	}

	_tZWaveDevice* pDevice = FindDevice(NodeID, instance, index, COMMAND_CLASS_SENSOR_BINARY, ZDTYPE_SWITCH_NORMAL);
	if (pDevice == NULL)
	{
		//one more try
		pDevice = FindDevice(NodeID, instance, index, COMMAND_CLASS_SWITCH_BINARY, ZDTYPE_SWITCH_NORMAL);
		if (pDevice == NULL)
		{
			// absolute last try
			instance = vID.GetIndex();
			pDevice = FindDevice(NodeID, -1, -1, COMMAND_CLASS_SENSOR_MULTILEVEL, ZDTYPE_SWITCH_NORMAL);
			if (pDevice == NULL)
			{
				//okey, 1 more
				int tmp_instance = index;
				pDevice = FindDevice(NodeID, tmp_instance, -1, COMMAND_CLASS_SWITCH_MULTILEVEL, ZDTYPE_SWITCH_DIMMER);
				if (pDevice == NULL)
				{
					return;
				}
			}
		}
	}

	int nintvalue = 0;
	if (EventID == 255)
		nintvalue = 255;
	else
		nintvalue = 0;

	//	if ((pDevice->intvalue == nintvalue) && (pDevice->sequence_number != 1))
	//	{
	//		//Do we need this ?
	//		return; //dont send/update same value
	//	}
	time_t atime = mytime(NULL);
	pDevice->intvalue = nintvalue;
	pDevice->lastreceived = atime;
	pDevice->sequence_number += 1;
	if (pDevice->sequence_number == 0)
		pDevice->sequence_number = 1;
	if (pDevice->bValidValue)
		SendDevice2Domoticz(pDevice);
}

void COpenZWave::UpdateValue(const OpenZWave::ValueID& vID)
{
	if (m_pManager == NULL)
		return;
	if (m_controllerID == 0)
		return;

	//	if (m_nodesQueried==false)
	//	return; //only allow updates when node Query is done
	uint8_t commandclass = vID.GetCommandClassId();
	unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();

	uint8_t instance = GetInstanceFromValueID(vID);

	uint8_t vOrgInstance = vID.GetInstance();
	uint16_t vOrgIndex = vID.GetIndex();

	OpenZWave::ValueID::ValueType vType = vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre = vID.GetGenre();
	std::string vLabel = m_pManager->GetValueLabel(vID);
	std::string vUnits = m_pManager->GetValueUnits(vID);

	float fValue = 0;
	int iValue = 0;
	bool bValue = false;
	uint8_t byteValue = 0;
	int16 shortValue = 0;
	int32 intValue = 0;
	std::string strValue = "";
	int32 lValue = 0;

	if (vType == OpenZWave::ValueID::ValueType_Decimal)
	{
		if (m_pManager->GetValueAsFloat(vID, &fValue) == false)
			return;
	}
	else if (vType == OpenZWave::ValueID::ValueType_Bool)
	{
		if (m_pManager->GetValueAsBool(vID, &bValue) == false)
			return;
	}
	else if (vType == OpenZWave::ValueID::ValueType_Byte)
	{
		if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
			return;
	}
	else if (vType == OpenZWave::ValueID::ValueType_Int)
	{
		if (m_pManager->GetValueAsInt(vID, &intValue) == false)
			return;
	}
	else if (vType == OpenZWave::ValueID::ValueType_Short)
	{
		if (m_pManager->GetValueAsShort(vID, &shortValue) == false)
			return;
	}
	else if ((vType == OpenZWave::ValueID::ValueType_Raw) || (vType == OpenZWave::ValueID::ValueType_String))
	{
		if (m_pManager->GetValueAsString(vID, &strValue) == false)
			return;
	}
	else if (vType == OpenZWave::ValueID::ValueType_List)
	{
		if (m_pManager->GetValueListSelection(vID, &lValue) == false)
			return;
	}
	else
	{
		_log.Log(LOG_ERROR, "OpenZWave: UpdateValue, Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		return;
	}

	if (vGenre != OpenZWave::ValueID::ValueGenre_User)
	{
		NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (pNode)
		{
			/*
			if (pNode->m_WasSleeping)
			{
			pNode->m_WasSleeping=false;
			m_pManager->RefreshNodeInfo(HomeID,NodeID);
			}
			*/
		}
		if ((pNode) && (vLabel.find("Wake-up Interval") != std::string::npos))
		{
			if (HomeID != m_controllerID)
				m_pManager->AddAssociation(HomeID, NodeID, 1, m_controllerNodeId);
		}
		else
		{
			//normal
			_log.Debug(DEBUG_HARDWARE, "OpenZWave: Value_Changed: (Not handling non-user genre: %d!) Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", vGenre, NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
		}
		return;
	}

	if (
		(vLabel.find("Exporting") != std::string::npos) ||
		(vLabel.find("Interval") != std::string::npos) ||
		(vLabel.find("Previous Reading") != std::string::npos)
		)
		return;

	if ((commandclass == COMMAND_CLASS_ALARM) || (commandclass == COMMAND_CLASS_SENSOR_ALARM))
	{
		instance = GetIndexFromAlarm(vLabel);
		if (instance == 0)
			return;
	}

	time_t atime = mytime(NULL);
	std::stringstream sstr;
	sstr << int(NodeID) << ".instances." << int(instance) << ".commandClasses." << int(commandclass) << ".data";

	if (
		(vLabel.find("Energy") != std::string::npos) ||
		(vLabel.find("Power") != std::string::npos) ||
		(vLabel.find("Voltage") != std::string::npos) ||
		(vLabel.find("Current") != std::string::npos) ||
		(vLabel.find("Power Factor") != std::string::npos) ||
		(vLabel.find("Gas") != std::string::npos) ||
		(vLabel.find("CO2 Level") != std::string::npos) ||
		(vLabel.find("Water") != std::string::npos) ||
		(vLabel.find("Moisture") != std::string::npos) ||
		(vLabel.find("Tank Capacity") != std::string::npos)
		)
	{
		int scaleID = 0;
		if (vLabel.find("Energy") != std::string::npos)
			scaleID = SCALEID_ENERGY;
		else if (vLabel.find("Power") != std::string::npos)
			scaleID = SCALEID_POWER;
		else if (vLabel.find("Voltage") != std::string::npos)
			scaleID = SCALEID_VOLTAGE;
		else if (vLabel.find("Current") != std::string::npos)
			scaleID = SCALEID_CURRENT;
		else if (vLabel.find("Power Factor") != std::string::npos)
			scaleID = SCALEID_POWERFACTOR;
		else if (vLabel.find("Gas") != std::string::npos)
			scaleID = SCALEID_GAS;
		else if (vLabel.find("CO2 Level") != std::string::npos)
			scaleID = SCALEID_CO2;
		else if (vLabel.find("Water") != std::string::npos)
			scaleID = SCALEID_WATER;
		else if (vLabel.find("Moisture") != std::string::npos)
			scaleID = SCALEID_MOISTRUE;
		else if (vLabel.find("Tank Capacity") != std::string::npos)
			scaleID = SCALEID_TANK_CAPACITY;
		else if (vLabel.find("Rain Rate") != std::string::npos)
			scaleID = SCALEID_RAIN_RATE;
		else if (vLabel.find("Seismic Intensity") != std::string::npos)
			scaleID = SCALEID_SEISMIC_INTENSITY;

		sstr << "." << scaleID;
	}
	std::string path = sstr.str();

#ifdef DEBUG_ZWAVE_INT
	_log.Log(LOG_NORM, "OpenZWave: Value_Changed: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
#endif

	if (commandclass == COMMAND_CLASS_USER_CODE)
	{
		if ((instance == 1) && (vGenre == OpenZWave::ValueID::ValueGenre_User) && (vID.GetIndex() == 0) && (vType == OpenZWave::ValueID::ValueType_Raw))
		{
			//Enrollment Code
			COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
			if (!pNode)
				return;
			if (pNode->Instances.find(1) == pNode->Instances.end())
				return; //no codes added yet, wake your tag reader

						//Check if we are in Enrollment Mode, if not dont continue

			if (!m_bInUserCodeEnrollmentMode)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Received new User Code/Tag, but we are not in Enrollment mode!");
				return;
			}
			m_bControllerCommandInProgress = false;

			bool bIncludedCode = false;

			for (std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.begin(); itt != pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.end(); ++itt)
			{
				OpenZWave::ValueID vNode = *itt;
				if ((vNode.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (vNode.GetInstance() == 1) && (vNode.GetType() == OpenZWave::ValueID::ValueType_Raw))
				{
					int vNodeIndex = vNode.GetIndex();
					if (vNodeIndex >= 1)
					{
						if (vType == OpenZWave::ValueID::ValueType_String)
						{
							std::string sValue;
							if (m_pManager->GetValueAsString(vNode, &sValue))
							{
								//Find Empty slot
								if (sValue.find("0x00 ") == 0)
								{
									_log.Log(LOG_STATUS, "OpenZWave: Enrolling User Code/Tag to code index: %d", vNodeIndex);
									m_pManager->SetValue(vNode, strValue);
									AddValue(vID, pNode);
									bIncludedCode = true;
									break;
								}
							}
						}
						else
						{
							_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
							return;
						}
					}
				}
			}
			if (!bIncludedCode)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Received new User Code/Tag, but there is no available room for new codes!!");
			}
			m_bInUserCodeEnrollmentMode = false;
			return;
		}
		else
		{
			uint16_t cIndex = vID.GetIndex();
			//if ((instance == 1) && (vGenre == OpenZWave::ValueID::ValueGenre_User) && (vID.GetIndex() != 0) && (vType == OpenZWave::ValueID::ValueType_Raw))
			//{
			//	_log.Log(LOG_NORM, "OpenZWave: Received User Code/Tag index: %d (%s)", cIndex, strValue.c_str());
			//}
		}
	}
	else if (commandclass == COMMAND_CLASS_BATTERY)
	{
		//Battery status update
		if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			UpdateDeviceBatteryStatus(NodeID, byteValue);
		}
		return;
	}
	else if (commandclass == COMMAND_CLASS_CLOCK)
	{
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (!pNode)
			return;

		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			if (vOrgIndex == ValueID_Index_Clock::Day)
			{
				try
				{
					int32 vDay;
					if (m_pManager->GetValueListSelection(vID, &vDay))
					{
						if (vDay > 0)
						{
							pNode->tClockDay = vDay - 1;
							return;
						}
					}
				}
				catch (...)
				{

				}
			}
		}
		else if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
				return;
			if (vOrgIndex == ValueID_Index_Clock::Hour)
			{
				pNode->tClockHour = byteValue;
				return;
			}
			else if (vOrgIndex == ValueID_Index_Clock::Minute)
			{
				pNode->tClockMinute = byteValue;
				if (
					(pNode->tClockDay != -1) &&
					(pNode->tClockHour != -1) &&
					(pNode->tClockMinute != -1)
					)
				{
					_log.Log(LOG_NORM, "OpenZWave: NodeID: %d (0x%02x), Thermostat Clock: %s %02d:%02d", NodeID, NodeID, ZWave_Clock_Days(pNode->tClockDay), pNode->tClockHour, pNode->tClockMinute);
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
			return;
		}
	}

	_tZWaveDevice* pDevice = NULL;
	std::map<std::string, _tZWaveDevice>::iterator itt;
	std::string path_plus = path + ".";
	for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
	{
		std::string dstring = itt->second.string_id;
		if (dstring == path) {
			pDevice = &itt->second;
			break;
		}
		else {
			size_t loc = dstring.find(path_plus);
			if (loc != std::string::npos)
			{
				pDevice = &itt->second;
				break;
			}
		}
	}
	if (pDevice == NULL)
	{
		//ignore the following command classes as they are not used in Domoticz at the moment
		if (
			(commandclass == COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE)
			)
		{
			return;
		}

		//New device, let's add it
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (!pNode)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Value_Changed: NodeID not found internally!. Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
			return;
		}

		AddValue(vID, pNode);
		for (itt = m_devices.begin(); itt != m_devices.end(); ++itt)
		{
			std::string dstring = itt->second.string_id;
			if (dstring == path) {
				pDevice = &itt->second;
				break;
			}
			else {
				size_t loc = dstring.find(path_plus);
				if (loc != std::string::npos)
				{
					pDevice = &itt->second;
					break;
				}
			}
		}
		if (pDevice == NULL)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Value_Changed: Tried adding value, not succeeded!. Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
			return;
		}
	}

	pDevice->bValidValue = true;
	pDevice->orgInstanceID = vOrgInstance;
	pDevice->orgIndexID = vOrgIndex;

	if (
		(pDevice->Manufacturer_id == 0) &&
		(pDevice->Product_id == 0) &&
		(pDevice->Product_type == 0)
		)
	{
		COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
		if (pNode)
		{
			if (!pNode->Manufacturer_name.empty())
			{
				pDevice->Manufacturer_id = pNode->Manufacturer_id;
				pDevice->Product_id = pNode->Product_id;
				pDevice->Product_type = pNode->Product_type;
			}
		}
	}

	switch (pDevice->devType)
	{
	case ZDTYPE_SWITCH_NORMAL:
	{
		if (commandclass == COMMAND_CLASS_SENSOR_ALARM)
		{
			int intValue = 0;
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				//1.4
				if (byteValue == 0)
					intValue = 0;
				else
					intValue = 255;
			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				//1.6
				int32 listValue = 0;
				if (m_pManager->GetValueListSelection(vID, &listValue))
				{
					if (listValue == 0)
						intValue = 0;
					else
						intValue = 255;
				}
			}
			pDevice->intvalue = intValue;
		}
		else if (commandclass == COMMAND_CLASS_ALARM)
		{
			// default
			int intValue = 0;
			int32 intListValue = 0;
			std::string szListValue;
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				//1.4
				if (byteValue == 0)
					intValue = 0;
				else
					intValue = 255;
				intListValue = intValue;
			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				//1.6
				if (m_pManager->GetValueListSelection(vID, &intListValue))
				{
					if (intListValue == 0)
						intValue = 0;
					else
						intValue = 255;
					m_pManager->GetValueListSelection(vID, &szListValue);
					_log.Log(LOG_STATUS, "OpenZWave: Alarm received (%s: %s)", vLabel.c_str(), szListValue.c_str());
				}
			}
			/*
			_log.Log(LOG_STATUS, "------------------------------------");
			_log.Log(LOG_STATUS, "Label: %s", vLabel.c_str());
			_log.Log(LOG_STATUS, "vOrgIndex: %d (0x%02x)", vOrgIndex, vOrgIndex);
			_log.Log(LOG_STATUS, "vOrgInstance: %d (0x%02x)", vOrgInstance, vOrgInstance);
			_log.Log(LOG_STATUS, "Value: %d (0x%02x)", intValue, intValue);
			_log.Log(LOG_STATUS, "------------------------------------");
			*/

			if (vLabel.find("Alarm Type") != std::string::npos)
			{
				if (intValue != 0)
				{
					m_LastAlarmTypeReceived = intValue;
				}
				else
					m_LastAlarmTypeReceived = -1;
			}
			else if (vLabel.find("Alarm Level") != std::string::npos)
			{
				if (m_LastAlarmTypeReceived != -1)
				{
					//Until we figured out what types/levels we have, we create a switch for each of them
					char szDeviceName[50];
					sprintf(szDeviceName, "Alarm Type: 0x%02X", m_LastAlarmTypeReceived);
					std::string tmpstr = szDeviceName;
					SendSwitch(NodeID, m_LastAlarmTypeReceived, pDevice->batValue, (intValue != 0) ? true : false, 0, tmpstr);
					m_LastAlarmTypeReceived = -1;
				}
			}
			else
			{
				int typeIndex = GetIndexFromAlarmType(vLabel);
				if (typeIndex > 0) //don't include reserver(0) type
				{
					char szTmp[100];
					sprintf(szTmp, "Alarm Type: %s %d (0x%02X)", vLabel.c_str(), typeIndex, typeIndex);
					SendZWaveAlarmSensor(pDevice->nodeID, pDevice->instanceID, pDevice->batValue, typeIndex, intListValue, szTmp);
				}

				if (
					(vLabel.find("Smoke") != std::string::npos) ||
					(vLabel.find("Carbon Monoxide") != std::string::npos) ||
					(vLabel.find("Carbon Dioxide") != std::string::npos) ||
					(vLabel.find("Heat") != std::string::npos) ||
					(vLabel.find("Flood") != std::string::npos) ||
					(vLabel.find("Water") != std::string::npos) ||
					(vLabel.find("Burglar") != std::string::npos) ||
					(vLabel.find("Home Security") != std::string::npos) ||
					(vLabel.find("Emergency") != std::string::npos) ||
					(vLabel.find("Appliance") != std::string::npos) ||
					(vLabel.find("HomeHealth") != std::string::npos) ||
					(vLabel.find("Home Health") != std::string::npos) ||
					(vLabel.find("Siren") != std::string::npos) ||
					(vLabel.find("Weather") != std::string::npos) ||
					(vLabel.find("Gas") != std::string::npos)
					)
				{
					switch (intValue) {
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vLabel.find("Access Control") != std::string::npos)
				{
					switch (intValue) {
						//ignore
					case 12:	// All User Codes Deleted
					case 13:	// Single User Code Deleted
					case 14:	// New User Code Added
					case 15:	// Duplicate User Code Not Added
					case 16:	// Keypad Disabled
					case 17:	// Keypad Busy
					case 18:	// New Program Code Entered
					case 19:	// User Codes Attempt Exceeds Limit
					case 20:	// Wireless Unlock Invalid UserCode Entered
					case 21:	// Wireless Lock Invalid UserCode Entered
					case 64:	// Barrier Initializing
					case 65:	// Barrier Force Exceeded
					case 66:	// Barrier Motor Time Exceeded
					case 67:	// Barrier Physical Limits Exceeded
					case 68:	// Barrier Failed Operation
					case 69:	// Barrier Unattended Operation Disabled
					case 70:	// Barrier Malfunction
					case 71:	// Barrier Vacaction Mode
					case 72:	// Barrier Saftey Beam Obstruction
					case 73:	// Barrier Sensor Not Detected
					case 74:	// Barrier Battery Low
					case 75:	// Barrier Short in Wall Station Wires
					case 76:	// Barrier Associated with Non ZWave Device
						pDevice->lastreceived = atime;
						return;
						//unlocked
					case 0x00: 	// Previous Events cleared
					case 0x02:	// Manual Unlock Operation
					case 0x04:	// Wireless Unlock Operation
					case 0x06:	// Keypad unlock/Arm Home
					case 0x17: 	// Door/Window closed
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
						//locked
					case 0x01:	// Manual Lock Operation
					case 0x03:	// Wireless Lock Operation
					case 0x05:	// Keypad Lock/Arm Away
					case 0x16: 	// Door/Window open
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vLabel.find("Power Management") != std::string::npos)
				{
					switch (intValue) {
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;

					case 0x0a:	// Replace battery soon
					case 0x0b:	// Replace battery now
					case 0x0e:	// Charge battery soon
					case 0x0f:	// Charge battery now!
						if (pDevice->hasBattery) {
							pDevice->batValue = 0; // signal battery needs attention ?!?
						}
						// fall through by intent
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vLabel.find("System") != std::string::npos)
				{
					switch (intValue) {
						//ignore
					case 0x05:	// Heartbeat
						pDevice->lastreceived = atime;
						return;
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vLabel.find("Clock") != std::string::npos)
				{
					switch (intValue) {
						//ignore
					case 0x03:	// Time Remaining
						pDevice->lastreceived = atime;
						return;
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (
					(vLabel.find("Alarm Type") == std::string::npos) &&
					(vLabel.find("Alarm Level") == std::string::npos)
					)
				{
					if (intValue != 0)
					{
						//Until we figured out what types/levels we have, we create a switch for each of them
						char szDeviceName[100];
						sprintf(szDeviceName, "Alarm Type: 0x%02X (%s)", intValue, vLabel.c_str());
						std::string tmpstr = szDeviceName;
						SendSwitch(NodeID, intValue, pDevice->batValue, true, 0, tmpstr);
					}
				}
			}
			pDevice->intvalue = intValue;
		}
		else if (vLabel.find("Open") != std::string::npos)
		{
			pDevice->intvalue = 255;
		}
		else if (vLabel.find("Close") != std::string::npos)
		{
			pDevice->intvalue = 0;
		}
		else
		{
			int intValue = 0;
			if (vType == OpenZWave::ValueID::ValueType_Bool)
			{
				if (bValue == true)
					intValue = 255;
				else
					intValue = 0;
			}
			else if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				if (byteValue == 0)
					intValue = 0;
				else
					intValue = 255;
			}
			else if (vType == OpenZWave::ValueID::ValueType_String)
			{
				if (commandclass == COMMAND_CLASS_COLOR_CONTROL)
				{
					//New color value received, not handled yet
				}

			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				if (commandclass == COMMAND_CLASS_COLOR_CONTROL)
				{
					//Color Index changed, not used
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Value_Changed: Unhandled value type ZDTYPE_SWITCH_NORMAL (%d). Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d (%s:%d)", vType, NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return;
			}
			if (pDevice->intvalue == intValue)
			{
				return; //dont send same value
			}
			pDevice->intvalue = intValue;
		}
	}
	break;
	case ZDTYPE_SWITCH_DIMMER:
		if (vLabel.find("Level") == std::string::npos)
			return;
		if (vType != OpenZWave::ValueID::ValueType_Byte)
			return;
		if (byteValue == 99)
			byteValue = 255;
		if (pDevice->intvalue == byteValue)
		{
			return; //dont send same value
		}
		pDevice->intvalue = byteValue;
		break;
	case ZDTYPE_SENSOR_POWER:
		if (
			(vLabel.find("Energy") == std::string::npos) &&
			(vLabel.find("Power") == std::string::npos)
			)
			return;
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue * pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_POWERENERGYMETER:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (
			(vLabel.find("Energy") == std::string::npos) &&
			(vLabel.find("Power") == std::string::npos)
			)
			return;
		pDevice->floatValue = fValue * pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_TEMPERATURE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Temperature") == std::string::npos)
			return;
		if (vUnits == "F")
		{
			//Convert to celcius
			fValue = float((fValue - 32) * (5.0 / 9.0));
		}
		if ((fValue < -200) || (fValue > 380))
			return;
		pDevice->bValidValue = (std::abs(pDevice->floatValue - fValue) < 10);
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_HUMIDITY:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Relative Humidity") == std::string::npos)
			return;
		pDevice->intvalue = round(fValue);
		break;
	case ZDTYPE_SENSOR_UV:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Ultraviolet") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_VELOCITY:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Velocity") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_BAROMETER:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (
			(vLabel.find("Barometric Pressure") != std::string::npos) ||
			(vLabel.find("Atmospheric Pressure") != std::string::npos)
			)
		{
			pDevice->floatValue = fValue * 10.0f;
		}
		else
			return;
		break;
	case ZDTYPE_SENSOR_DEWPOINT:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Dew Point") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_LIGHT:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Luminance") == std::string::npos)
			return;
		if (vUnits != "lux")
		{
			//convert from % to Lux (where max is 1000 Lux)
			fValue = (1000.0f / 100.0f) * fValue;
		}
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_VOLTAGE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Voltage") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_AMPERE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Current") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_SETPOINT:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vUnits == "F")
		{
			//Convert to celcius
			fValue = float((fValue - 32) * (5.0 / 9.0));
		}
		pDevice->bValidValue = (std::abs(pDevice->floatValue - fValue) < 10);
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_PERCENTAGE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Power Factor") == std::string::npos)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_GAS:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Gas") == std::string::npos)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue; //always set the value
		if ((fValue - oldvalue > 10.0f) || (fValue < oldvalue))
			return;//sanity check, don't report it
	}
	break;
	case ZDTYPE_SENSOR_CO2:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("CO2 Level") == std::string::npos)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue; //always set the value
	}
	break;
	case ZDTYPE_SENSOR_WATER:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Water") == std::string::npos)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue; //always set the value
	}
	break;
	case ZDTYPE_SENSOR_CUSTOM:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_MOISTURE:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Moisture") == std::string::npos)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue; //always set the value
	}
	break;
	case ZDTYPE_SENSOR_TANK_CAPACITY:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel.find("Tank Capacity") == std::string::npos)
			return;
		float oldvalue = pDevice->floatValue;
		pDevice->floatValue = fValue; //always set the value
	}
	break;
	case ZDTYPE_SENSOR_THERMOSTAT_CLOCK:
		if (vOrgIndex == ValueID_Index_Clock::Minute)
		{
			COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
			if (!pNode)
				return;
			pDevice->intvalue = (pNode->tClockDay * (24 * 60)) + (pNode->tClockHour * 60) + pNode->tClockMinute;
		}
		break;
	case ZDTYPE_SENSOR_THERMOSTAT_MODE:
		if (vType != OpenZWave::ValueID::ValueType_List)
			return;
		if (vOrgIndex == ValueID_Index_ThermostatMode::Mode)
		{
			COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
			if (!pNode)
				return;
			pNode->tModes.clear();
			m_pManager->GetValueListItems(vID, &pNode->tModes);
			if (!pNode->tModes.empty())
			{
				std::string vListStr;
				if (!m_pManager->GetValueListSelection(vID, &vListStr))
					return;
				int32 lValue = Lookup_ZWave_Thermostat_Modes(pNode->tModes, vListStr);
				if (lValue == -1)
					return;
				pNode->tMode = lValue;
				pDevice->intvalue = lValue;
			}
		}
		break;
	case ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE:
		if (vType != OpenZWave::ValueID::ValueType_List)
			return;
		if (vOrgIndex == ValueID_Index_ThermostatFanMode::FanMode)
		{
			COpenZWave::NodeInfo* pNode = GetNodeInfo(HomeID, NodeID);
			if (!pNode)
				return;
			pNode->tFanModes.clear();
			m_pManager->GetValueListItems(vID, &pNode->tFanModes);
			try
			{
				std::string vListStr;
				if (!m_pManager->GetValueListSelection(vID, &vListStr))
					return;
				int32 lValue = Lookup_ZWave_Thermostat_Fan_Modes(vListStr);
				if (lValue == -1)
					return;
				pNode->tFanMode = lValue;
				pDevice->intvalue = lValue;
			}
			catch (...)
			{

			}
		}
		break;
	case ZDTYPE_CENTRAL_SCENE:
		if (commandclass == COMMAND_CLASS_SCENE_ACTIVATION)
		{
			if (vOrgIndex != ValueID_Index_SceneActivation::SceneID)
				return;
			if (vType != OpenZWave::ValueID::ValueType_Int)
				return; //not interested in you
			if (m_pManager->GetValueAsInt(vID, &intValue) == false)
				return;
			if (intValue == 0)
				return; //invalid SceneID (should we ignore it)
			pDevice->intvalue = intValue;
		}
		else
		{
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				pDevice->intvalue = 255;
			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				int32 listValue = 0;
				if (m_pManager->GetValueListSelection(vID, &listValue))
				{
					if ((lValue == 0) || (lValue == 2)) // CentralSceneMask_KeyReleased
						return;
					pDevice->intvalue = lValue;
				}
			}
		}
		break;
	}

	pDevice->lastreceived = atime;
	pDevice->sequence_number += 1;
	if (pDevice->sequence_number == 0)
		pDevice->sequence_number = 1;
	if (pDevice->bValidValue)
		SendDevice2Domoticz(pDevice);
}

bool COpenZWave::IncludeDevice(const bool bSecure)
{
	if (m_pManager == NULL)
		return false;
	try
	{
		CancelControllerCommand();
		m_LastIncludedNode = 0;
		m_LastIncludedNodeType = "";
		m_bHaveLastIncludedNodeInfo = false;
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = false; //unlimited time
		m_pManager->AddNode(m_controllerID, bSecure);
		_log.Log(LOG_STATUS, "OpenZWave: Node Include command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::ExcludeDevice(const int nodeID)
{
	try
	{
		if (m_pManager == NULL)
			return false;
		CancelControllerCommand();
		m_LastRemovedNode = 0;
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = false; //unlimited time
		m_pManager->RemoveNode(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Node Exclude command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::IsNodeIncluded()
{
	return ((m_LastIncludedNode != 0) && (m_bHaveLastIncludedNodeInfo == true));
}

bool COpenZWave::IsNodeExcluded()
{
	return (m_LastRemovedNode != 0);
}


bool COpenZWave::SoftResetDevice()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_pManager->SoftReset(m_controllerID);
		m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (HardwareID = %d)", m_HwdID);
		_log.Log(LOG_STATUS, "OpenZWave: Soft Reset device executed...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::HardResetDevice()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_pManager->ResetController(m_controllerID);
		m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (HardwareID = %d)", m_HwdID);
		_log.Log(LOG_STATUS, "OpenZWave: Hard Reset device executed...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::HealNetwork()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_pManager->HealNetwork(m_controllerID, true);
		_log.Log(LOG_STATUS, "OpenZWave: Heal Network command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::HealNode(const int nodeID)
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_pManager->HealNetworkNode(m_controllerID, nodeID, true);
		_log.Log(LOG_STATUS, "OpenZWave: Heal Node command initiated for node: %d (0x%02x)...", nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::NetworkInfo(const int hwID, std::vector< std::vector< int > >& NodeArray)
{
	if (m_pManager == NULL) {
		return false;
	}

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT HomeID,NodeID FROM ZWaveNodes WHERE (HardwareID = %d)",
		hwID);
	if (result.empty()) {
		return false;
	}
	int rowCnt = 0;
	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;
		int nodeID = atoi(sd[1].c_str());
		unsigned int homeID = static_cast<unsigned int>(std::stoul(sd[0]));
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == NULL)
			continue;

		std::vector<int> row;
		NodeArray.push_back(row);
		NodeArray[rowCnt].push_back(nodeID);
		uint8* arr;

		try
		{
			int retval = m_pManager->GetNodeNeighbors(homeID, nodeID, &arr);
			if (retval > 0) {

				for (int i = 0; i < retval; i++) {
					NodeArray[rowCnt].push_back(arr[i]);
				}

				delete[] arr;
			}
		}
		catch (OpenZWave::OZWException& ex)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
			return false;
		}
		rowCnt++;
	}
	return true;
}

int COpenZWave::ListGroupsForNode(const int nodeID)
{
	try
	{
		if (m_pManager == NULL)
			return 0;
		return m_pManager->GetNumGroups(m_controllerID, nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return 0;
}

std::string COpenZWave::GetGroupName(const int nodeID, const int groupID)
{
	if (m_pManager == NULL)
		return "";

	try
	{
		return m_pManager->GetGroupLabel(m_controllerID, nodeID, groupID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return "";
}

int COpenZWave::ListAssociatedNodesinGroup(const int nodeID, const int groupID, std::vector<std::string>& nodesingroup)
{

	if (m_pManager == NULL)
		return 0;

	try
	{
		OpenZWave::InstanceAssociation* arr;
		int retval = m_pManager->GetAssociations(m_controllerID, nodeID, groupID, &arr);
		if (retval > 0) {
			for (int i = 0; i < retval; i++) {
				char str[32];
				if (arr[i].m_instance == 0) {
					snprintf(str, 32, "%d", arr[i].m_nodeId);
				}
				else {
					snprintf(str, 32, "%d.%d", arr[i].m_nodeId, arr[i].m_instance);
				}
				nodesingroup.push_back(str);
			}
			delete[] arr;
		}
		return retval;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return 0;
}

bool COpenZWave::AddNodeToGroup(const int nodeID, const int groupID, const int addID, const int instance)
{

	if (m_pManager == NULL)
		return false;
	try
	{
		m_pManager->AddAssociation(m_controllerID, nodeID, groupID, addID, instance);
		_log.Log(LOG_STATUS, "OpenZWave: added node: %d (0x%02x) instance %d in group: %d of node: %d (0x%02x)", addID, addID, instance, groupID, nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::RemoveNodeFromGroup(const int nodeID, const int groupID, const int removeID, const int instance)
{
	if (m_pManager == NULL)
		return false;
	try
	{
		m_pManager->RemoveAssociation(m_controllerID, nodeID, groupID, removeID, instance);
		_log.Log(LOG_STATUS, "OpenZWave: removed node: %d (0x%02x) instance %d from group: %d of node: %d (0x%02x)", removeID, removeID, instance, groupID, nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::RemoveFailedDevice(const int nodeID)
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->RemoveFailedNode(m_controllerID, (uint8_t)nodeID);
		_log.Log(LOG_STATUS, "OpenZWave: Remove Failed Device initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::ReceiveConfigurationFromOtherController()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->ReceiveConfiguration(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Receive Configuration initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::SendConfigurationToSecondaryController()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->ReplicationSend(m_controllerID, 0xFF);
		_log.Log(LOG_STATUS, "OpenZWave: Replication to Secondary Controller initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::TransferPrimaryRole()
{
	if (m_pManager == NULL)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(NULL);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->TransferPrimaryRole(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Transfer Primary Role initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::CancelControllerCommand(const bool bForce)
{
	if ((m_bControllerCommandInProgress == false) && (!bForce))
		return false;
	if (m_pManager == NULL)
		return false;

	m_bControllerCommandInProgress = false;
	m_bControllerCommandCanceled = true;
	try
	{
		return m_pManager->CancelControllerCommand(m_controllerID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

void COpenZWave::GetConfigFile(std::string& filePath, std::string& fileContent)
{
	std::string retstring = "";
	if (m_pManager == NULL)
		return;

	std::lock_guard<std::mutex> l(m_NotificationMutex);

	char szFileName[255];
	sprintf(szFileName, "%sConfig/zwcfg_0x%08x.xml", szUserDataFolder.c_str(), m_controllerID);
	filePath = szFileName;

	std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
	if (file)
	{
		fileContent.append((std::istreambuf_iterator<char>(file)),
			(std::istreambuf_iterator<char>()));
		//file.close();
	}
}

void COpenZWave::OnZWaveDeviceStatusUpdate(int _cs, int _err)
{
	OpenZWave::Driver::ControllerState cs = (OpenZWave::Driver::ControllerState)_cs;
	OpenZWave::Driver::ControllerError err = (OpenZWave::Driver::ControllerError)_err;

	std::string szLog;

	switch (cs)
	{
	case OpenZWave::Driver::ControllerState_Normal:
		m_bControllerCommandInProgress = false;
		szLog = "No Command in progress";
		break;
	case OpenZWave::Driver::ControllerState_Starting:
		szLog = "Starting controller command";
		break;
	case OpenZWave::Driver::ControllerState_Cancel:
		szLog = "The command was canceled";
		m_bControllerCommandInProgress = false;
		break;
	case OpenZWave::Driver::ControllerState_Error:
		szLog = "Command invocation had error(s) and was aborted";
		m_bControllerCommandInProgress = false;
		break;
	case OpenZWave::Driver::ControllerState_Waiting:
		m_bControllerCommandInProgress = true;
		szLog = "Controller is waiting for a user action";
		break;
	case OpenZWave::Driver::ControllerState_Sleeping:
		szLog = "Controller command is on a sleep queue wait for device";
		break;
	case OpenZWave::Driver::ControllerState_InProgress:
		szLog = "The controller is communicating with the other device to carry out the command";
		break;
	case OpenZWave::Driver::ControllerState_Completed:
		m_bControllerCommandInProgress = false;
		if (!m_bControllerCommandCanceled)
		{
			szLog = "The command has completed successfully";
		}
		else
		{
			szLog = "The command was canceled";
		}
		break;
	case OpenZWave::Driver::ControllerState_Failed:
		szLog = "The command has failed";
		m_bControllerCommandInProgress = false;
		break;
	case OpenZWave::Driver::ControllerState_NodeOK:
		szLog = "Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK";
		break;
	case OpenZWave::Driver::ControllerState_NodeFailed:
		szLog = "Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed";
		break;
	default:
		szLog = "Unknown Device Response!";
		m_bControllerCommandInProgress = false;
		break;
	}
	_log.Log(LOG_STATUS, "OpenZWave: Device Response: %s", szLog.c_str());
}

void COpenZWave::EnableNodePoll(const unsigned int homeID, const int nodeID, const int pollTime)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == NULL)
			return; //Not found

		bool bSingleIndexPoll = false;

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT ProductDescription FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)",
			m_HwdID, homeID, nodeID);
		if (!result.empty())
		{
			std::string ProductDescription = result[0][0];
			bSingleIndexPoll = (
				(ProductDescription.find("GreenWave PowerNode 6 port") != std::string::npos)
				);
		}

		for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance = pNode->Instances.begin(); ittInstance != pNode->Instances.end(); ++ittInstance)
		{
			for (std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds)
			{
				for (std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue != ittCmds->second.Values.end(); ++ittValue)
				{
					uint8_t commandclass = ittValue->GetCommandClassId();
					OpenZWave::ValueID::ValueGenre vGenre = ittValue->GetGenre();

					unsigned int _homeID = ittValue->GetHomeId();
					uint8_t _nodeID = ittValue->GetNodeId();
					uint16_t vOrgIndex = ittValue->GetIndex();
					if (m_pManager->IsNodeFailed(_homeID, _nodeID))
					{
						//do not enable/disable polling on nodes that are failed
						continue;
					}

					//Ignore non-user types
					if (vGenre != OpenZWave::ValueID::ValueGenre_User)
						continue;

					std::string vLabel = m_pManager->GetValueLabel(*ittValue);

					if (
						(vLabel.find("Exporting") != std::string::npos) ||
						(vLabel.find("Interval") != std::string::npos) ||
						(vLabel.find("Previous Reading") != std::string::npos)
						)
						continue;

					if (commandclass == COMMAND_CLASS_SWITCH_BINARY)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_SWITCH_MULTILEVEL)
					{
						if (vOrgIndex == ValueID_Index_SwitchMultiLevel::Level)
						{
							if (!m_pManager->isPolled(*ittValue))
								m_pManager->EnablePoll(*ittValue, 1);
						}
					}
					else if (commandclass == COMMAND_CLASS_SENSOR_BINARY)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_METER)
					{
						//Meter device
						if (
							(vLabel.find("Energy") != std::string::npos) ||
							(vLabel.find("Power") != std::string::npos) ||
							(vLabel.find("Gas") != std::string::npos) ||
							(vLabel.find("Water") != std::string::npos)
							)
						{
							if (bSingleIndexPoll && (ittValue->GetIndex() != 0))
								continue;
							if (!m_pManager->isPolled(*ittValue))
								m_pManager->EnablePoll(*ittValue, 1);
						}
					}
					else if (commandclass == COMMAND_CLASS_SENSOR_MULTILEVEL)
					{
						//if ((*ittValue).GetIndex() != 0)
						//{
						//	continue;
						//}
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 2);
					}
					else if (commandclass == COMMAND_CLASS_BATTERY)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 2);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_SETPOINT)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_MODE)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_STATE)
					{
						if (!m_pManager->isPolled(*ittValue))
							m_pManager->EnablePoll(*ittValue, 1);
					}
					else
					{
						m_pManager->DisablePoll(*ittValue);
					}
				}
			}
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::DisableNodePoll(const unsigned int homeID, const int nodeID)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == NULL)
			return; //Not found

		for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance = pNode->Instances.begin(); ittInstance != pNode->Instances.end(); ++ittInstance)
		{
			for (std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds)
			{
				for (std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue != ittCmds->second.Values.end(); ++ittValue)
				{
					if (m_pManager->isPolled(*ittValue))
						m_pManager->DisablePoll(*ittValue);
				}
			}
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::DeleteNode(const unsigned int homeID, const int nodeID)
{
	m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)",	m_HwdID, homeID, nodeID);
}

void COpenZWave::AddNode(const unsigned int homeID, const int nodeID, const NodeInfo* pNode)
{
	if (m_controllerID == 0)
		return;
	if (homeID != m_controllerID)
		return;
	//Check if node already exist
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)",
		m_HwdID, homeID, nodeID);
	std::string sProductDescription;
	if (!pNode->Manufacturer_name.empty() || !pNode->Product_name.empty())
		sProductDescription = pNode->Manufacturer_name + " " + pNode->Product_name;

	if (result.empty())
	{
		//Not Found, Add it to the database
		if (nodeID != m_controllerNodeId)
			m_sql.safe_query("INSERT INTO ZWaveNodes (HardwareID, HomeID, NodeID, ProductDescription) VALUES (%d,%u,%d,'%q')",
				m_HwdID, homeID, nodeID, sProductDescription.c_str());
		else
			m_sql.safe_query("INSERT INTO ZWaveNodes (HardwareID, HomeID, NodeID, Name,ProductDescription) VALUES (%d,%u,%d,'Controller','%q')",
				m_HwdID, homeID, nodeID, sProductDescription.c_str());
		_log.Log(LOG_STATUS, "OpenZWave: New Node added. HomeID: %u, NodeID: %d (0x%02x)", homeID, nodeID, nodeID);
	}
	else
	{
		if (
			(pNode->Manufacturer_name.size() == 0) ||
			(pNode->Product_name.size() == 0)
			)
			return;
		//Update ProductDescription
		m_sql.safe_query("UPDATE ZWaveNodes SET ProductDescription='%q' WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)",
			sProductDescription.c_str(), m_HwdID, homeID, nodeID);
	}
}

std::string COpenZWave::GetVersion()
{
	if (!m_pManager)
		return "?Manager Down!";
	return m_pManager->getVersionAsString();
}

std::string COpenZWave::GetVersionLong()
{
	if (!m_pManager)
		return "?Manager Down!";
	return m_pManager->getVersionLongAsString();
}

void COpenZWave::SetNodeName(const unsigned int homeID, const int nodeID, const std::string& Name)
{
	try
	{
		std::string NodeName = Name;
		if (NodeName.size() > 16)
			NodeName = NodeName.substr(0, 15);
		stdreplace(NodeName, "\"", "_");
		m_pManager->SetNodeName(homeID, nodeID, NodeName);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::EnableDisableNodePolling(const int nodeID)
{
	if (!m_pManager)
		return;

	try
	{
		int intervalseconds = 60;
		m_sql.GetPreferencesVar("ZWavePollInterval", intervalseconds);

		m_pManager->SetPollInterval(intervalseconds * 1000, false);

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT PollTime FROM ZWaveNodes WHERE (HardwareID==%d) AND (NodeID==%d)",
			m_HwdID, nodeID);
		if (result.empty())
			return;
		int PollTime = atoi(result[0][0].c_str());

		if (PollTime > 0)
			EnableNodePoll(m_controllerID, nodeID, PollTime);
		else
			DisableNodePoll(m_controllerID, nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::SetClock(const int nodeID, const int instanceID, const int commandClass, const int day, const int hour, const int minute)
{
	if (!m_pManager)
		return;

	//We have to set 3 values here (Later check if we can use COMMAND_CLASS_MULTI_CMD for this to do it in one)

	NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
		return;

	OpenZWave::ValueID vDay(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
	OpenZWave::ValueID vHour(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
	OpenZWave::ValueID vMinute(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);


	if (GetValueByCommandClassIndex(nodeID, 1, COMMAND_CLASS_CLOCK, ValueID_Index_Clock::Day, vDay) == false)
		return;
	if (GetValueByCommandClassIndex(nodeID, 1, COMMAND_CLASS_CLOCK, ValueID_Index_Clock::Hour, vHour) == false)
		return;
	if (GetValueByCommandClassIndex(nodeID, 1, COMMAND_CLASS_CLOCK, ValueID_Index_Clock::Minute, vMinute) == false)
		return;

	try
	{
		m_pManager->SetValueListSelection(vDay, ZWave_Clock_Days(day));
		m_pManager->SetValue(vHour, (const uint8)hour);
		m_pManager->SetValue(vMinute, (const uint8)minute);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::SetThermostatMode(const int nodeID, const int instanceID, const int commandClass, const int tMode)
{
	if (m_pManager == NULL)
		return;

	try
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			int nodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
			if (pNode)
			{
				if (tMode < (int)pNode->tModes.size())
				{
					m_pManager->SetValueListSelection(vID, pNode->tModes[tMode]);
				}
			}
		}
		m_updateTime = mytime(NULL);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

void COpenZWave::SetThermostatFanMode(const int nodeID, const int instanceID, const int commandClass, const int fMode)
{
	if (m_pManager == NULL)
		return;

	try
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_FAN_MODE, vID) == true)
		{
			m_pManager->SetValueListSelection(vID, ZWave_Thermostat_Fan_Modes[fMode]);
		}
		m_updateTime = mytime(NULL);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

std::vector<std::string> COpenZWave::GetSupportedThermostatModes(const unsigned long ID)
{
	std::lock_guard<std::mutex> l(m_NotificationMutex);
	std::vector<std::string> ret;
	uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
	uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
	uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
	uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

	int nodeID = (ID2 << 8) | ID3;
	int instanceID = ID4;
	int indexID = ID1;

	const _tZWaveDevice* pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_MODE);
	if (pDevice)
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			int nodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
			if (pNode)
			{
				return pNode->tModes;
			}
		}
	}
	return ret;
}

std::string COpenZWave::GetSupportedThermostatFanModes(const unsigned long ID)
{
	std::lock_guard<std::mutex> l(m_NotificationMutex);
	std::string retstr = "";
	uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
	uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
	uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
	uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

	int nodeID = (ID2 << 8) | ID3;
	int instanceID = ID4;
	int indexID = ID1;

	const _tZWaveDevice* pDevice = FindDevice(nodeID, instanceID, indexID, ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE);
	if (pDevice)
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_FAN_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			int nodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
			if (pNode)
			{
				int smode = 0;
				char szTmp[200];
				std::string modes = "";
				while (ZWave_Thermostat_Fan_Modes[smode] != NULL)
				{
					if (std::find(pNode->tFanModes.begin(), pNode->tFanModes.end(), ZWave_Thermostat_Fan_Modes[smode]) != pNode->tFanModes.end())
					{
						//Value supported
						sprintf(szTmp, "%d;%s;", smode, ZWave_Thermostat_Fan_Modes[smode]);
						modes += szTmp;
					}
					smode++;
				}
				retstr = modes;
			}
		}
	}
	return retstr;
}

void COpenZWave::NodeQueried(const unsigned int homeID, const int nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == NULL)
		return;

	try
	{
		//All nodes have been queried, enable/disable node polling
		pNode->IsPlus = m_pManager->IsNodeZWavePlus(homeID, nodeID);
		EnableDisableNodePolling(nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

bool COpenZWave::RequestNodeConfig(const unsigned int homeID, const int nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == NULL)
		return false;

	try
	{
		m_pManager->RequestAllConfigParams(homeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::RequestNodeInfo(const unsigned int homeID, const int nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == NULL)
		return false;

	try
	{
		m_pManager->RefreshNodeInfo(homeID, (uint8)nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

void COpenZWave::GetNodeValuesJson(const unsigned int homeID, const int nodeID, Json::Value& root, const int index)
{
	if (!m_pManager)
		return;

	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == NULL)
			return;

		int ivalue = 0;
		int vIndex = 1;
		std::string szValue;

		if (nodeID == m_controllerNodeId)
		{
			//Main ZWave node

			//Poll Interval
			root["result"][index]["config"][ivalue]["type"] = "short";
			int intervalseconds = 60;
			m_sql.GetPreferencesVar("ZWavePollInterval", intervalseconds);
			root["result"][index]["config"][ivalue]["value"] = intervalseconds;

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Poll Interval";
			root["result"][index]["config"][ivalue]["units"] = "Seconds";
			root["result"][index]["config"][ivalue]["help"] =
				"Set the time period between polls of a node's state. The length of the interval is the same for all devices. "
				"To even out the Z-Wave network traffic generated by polling, OpenZWave divides the polling interval by the number of devices that have polling enabled, and polls each "
				"in turn. It is recommended that if possible, the interval should not be set shorter than the number of polled devices in seconds (so that the network does not have to cope with more than one poll per second).";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			ivalue++;

			//Debug
			root["result"][index]["config"][ivalue]["type"] = "list";
			int debugenabled = 0;
			m_sql.GetPreferencesVar("ZWaveEnableDebug", debugenabled);

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Enable Debug";
			root["result"][index]["config"][ivalue]["units"] = "";
			root["result"][index]["config"][ivalue]["help"] =
				"Enable/Disable debug logging. "
				"It is not recommended to enable Debug for a live system as the log files generated will grow large quickly.";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";

			root["result"][index]["config"][ivalue]["list_items"] = 2;
			root["result"][index]["config"][ivalue]["listitem"][0] = "Disabled";
			root["result"][index]["config"][ivalue]["listitem"][1] = "Enabled";
			root["result"][index]["config"][ivalue]["value"] = (debugenabled == 0) ? "Disabled" : "Enabled";
			ivalue++;

			//Nightly Node Heal
			root["result"][index]["config"][ivalue]["type"] = "list";

			int nightly_heal = 0;
			m_sql.GetPreferencesVar("ZWaveEnableNightlyNetworkHeal", nightly_heal);

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Enable Nightly Heal Network (04:00 am)";
			root["result"][index]["config"][ivalue]["units"] = "";
			root["result"][index]["config"][ivalue]["help"] = "Enable/Disable nightly heal network.";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			root["result"][index]["config"][ivalue]["list_items"] = 2;
			root["result"][index]["config"][ivalue]["listitem"][0] = "Disabled";
			root["result"][index]["config"][ivalue]["listitem"][1] = "Enabled";
			root["result"][index]["config"][ivalue]["value"] = (nightly_heal == 0) ? "Disabled" : "Enabled";
			ivalue++;

			//Network Key
			root["result"][index]["config"][ivalue]["type"] = "string";

			std::string sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
			m_sql.GetPreferencesVar("ZWaveNetworkKey", sValue);
			std::vector<std::string> splitresults;
			StringSplit(sValue, ",", splitresults);
			if (splitresults.size() != 16)
			{
				sValue = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
			}
			root["result"][index]["config"][ivalue]["value"] = sValue;

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Security Network Key";
			root["result"][index]["config"][ivalue]["units"] = "";
			root["result"][index]["config"][ivalue]["help"] =
				"If you are using any Security Devices, you MUST set a network Key "
				"The length should be 16 bytes!";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			ivalue++;

			//RetryTimeout
			root["result"][index]["config"][ivalue]["type"] = "short";
			int nValue = 3000;
			m_sql.GetPreferencesVar("ZWaveRetryTimeout", nValue);
			root["result"][index]["config"][ivalue]["value"] = nValue;

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "RetryTimeout";
			root["result"][index]["config"][ivalue]["units"] = "ms"; //
			root["result"][index]["config"][ivalue]["help"] = "How long do we wait to timeout messages sent (default 10000)";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			ivalue++;

			//AssumeAwake
			root["result"][index]["config"][ivalue]["type"] = "list";
			nValue = 0;
			m_sql.GetPreferencesVar("ZWaveAssumeAwake", nValue);

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Assume Awake";
			root["result"][index]["config"][ivalue]["units"] = "";
			root["result"][index]["config"][ivalue]["help"] = "Assume Devices that Support the Wakeup CC are awake when we first query them.... (default True)";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			root["result"][index]["config"][ivalue]["list_items"] = 2;
			root["result"][index]["config"][ivalue]["listitem"][0] = "False";
			root["result"][index]["config"][ivalue]["listitem"][1] = "True";
			root["result"][index]["config"][ivalue]["value"] = (nValue == 0) ? "False" : "True";
			ivalue++;

			//PerformReturnRoutes
			root["result"][index]["config"][ivalue]["type"] = "list";
			nValue = 1;
			m_sql.GetPreferencesVar("ZWavePerformReturnRoutes", nValue);

			root["result"][index]["config"][ivalue]["index"] = vIndex++;
			root["result"][index]["config"][ivalue]["label"] = "Perform Return Routes";
			root["result"][index]["config"][ivalue]["units"] = "";
			root["result"][index]["config"][ivalue]["help"] = "if True, return routes will be updated";
			root["result"][index]["config"][ivalue]["LastUpdate"] = "-";
			root["result"][index]["config"][ivalue]["list_items"] = 2;
			root["result"][index]["config"][ivalue]["listitem"][0] = "False";
			root["result"][index]["config"][ivalue]["listitem"][1] = "True";
			root["result"][index]["config"][ivalue]["value"] = (nValue == 0) ? "False" : "True";
			ivalue++;

			//Aeotec Blinking state
			uint16_t Manufacturer_id = GetUInt16FromString(m_pManager->GetNodeManufacturerId(m_controllerID, m_controllerNodeId));
			uint16_t Product_type = GetUInt16FromString(m_pManager->GetNodeProductType(m_controllerID, m_controllerNodeId));
			uint16_t Product_id = GetUInt16FromString(m_pManager->GetNodeProductId(m_controllerID, m_controllerNodeId));

			if (Manufacturer_id == 0x0086)
			{
				if (
					((Product_type == 0x0001) && (Product_id == 0x005a)) || //Z-Stick Gen5
					((Product_type == 0x0001) && (Product_id == 0x005c)) || //Z-Stick Lite Gen5
					((Product_type == 0x0101) && (Product_id == 0x005a)) || //Z-Stick Gen5
					((Product_type == 0x0201) && (Product_id == 0x005a))    //Z-Stick Gen5
					)
				{
					int blinkenabled = 1;
					m_sql.GetPreferencesVar("ZWaveAeotecBlinkEnabled", blinkenabled);

					root["result"][index]["config"][ivalue]["index"] = vIndex;
					root["result"][index]["config"][ivalue]["type"] = "list";
					root["result"][index]["config"][ivalue]["units"] = "";
					root["result"][index]["config"][ivalue]["label"] = "Enable Controller Blinking";
					root["result"][index]["config"][ivalue]["help"] = "Enable/Disable controller blinking colors when plugged in USB.";
					root["result"][index]["config"][ivalue]["LastUpdate"] = "-";

					root["result"][index]["config"][ivalue]["list_items"] = 2;
					root["result"][index]["config"][ivalue]["listitem"][0] = "Disabled";
					root["result"][index]["config"][ivalue]["listitem"][1] = "Enabled";
					root["result"][index]["config"][ivalue]["value"] = (blinkenabled == 0) ? "Disabled" : "Enabled";
				}
			}
			vIndex++;
			ivalue++;


			return;
		}

		for (auto const& ittInstance : pNode->Instances)
		{
			for (auto const& ittCmds : ittInstance.second)
			{
				for (auto const& ittValue : ittCmds.second.Values)
				{
					uint8_t commandclass = ittValue.GetCommandClassId();
					if ((commandclass == COMMAND_CLASS_CONFIGURATION) || (commandclass == COMMAND_CLASS_PROTECTION))
					{
						if (m_pManager->IsValueReadOnly(ittValue) == true)
							continue;

						OpenZWave::ValueID::ValueType vType = ittValue.GetType();

						if (vType == OpenZWave::ValueID::ValueType_Decimal)
						{
							root["result"][index]["config"][ivalue]["type"] = "float";
						}
						else if (vType == OpenZWave::ValueID::ValueType_Bool)
						{
							root["result"][index]["config"][ivalue]["type"] = "bool";
						}
						else if (vType == OpenZWave::ValueID::ValueType_Byte)
						{
							root["result"][index]["config"][ivalue]["type"] = "byte";
						}
						else if (vType == OpenZWave::ValueID::ValueType_Short)
						{
							root["result"][index]["config"][ivalue]["type"] = "short";
						}
						else if (vType == OpenZWave::ValueID::ValueType_Int)
						{
							root["result"][index]["config"][ivalue]["type"] = "int";
						}
						else if (vType == OpenZWave::ValueID::ValueType_Button)
						{
							//root["result"][index]["config"][ivalue]["type"]="button";
							//Not supported now
							continue;
						}
						else if (vType == OpenZWave::ValueID::ValueType_BitSet)
						{
							root["result"][index]["config"][ivalue]["type"] = "int"; //int32
						}
						else if (vType == OpenZWave::ValueID::ValueType_List)
						{
							root["result"][index]["config"][ivalue]["type"] = "list";
						}
						else
						{
							//not supported
							_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
							continue;
						}

						if (m_pManager->GetValueAsString(ittValue, &szValue) == false)
						{
							continue;
						}
						root["result"][index]["config"][ivalue]["value"] = szValue;
						if (vType == OpenZWave::ValueID::ValueType_List)
						{
							std::vector<std::string> strs;
							m_pManager->GetValueListItems(ittValue, &strs);
							root["result"][index]["config"][ivalue]["list_items"] = static_cast<int>(strs.size());
							int vcounter = 0;
							for (std::vector<std::string>::const_iterator it = strs.begin(); it != strs.end(); ++it)
							{
								root["result"][index]["config"][ivalue]["listitem"][vcounter++] = *it;
							}
						}
						uint16_t i_index = ittValue.GetIndex();
						if (commandclass == COMMAND_CLASS_PROTECTION)
						{
							i_index = 3000 + ittValue.GetInstance();
						}

						std::string i_label = m_pManager->GetValueLabel(ittValue);
						std::string i_units = m_pManager->GetValueUnits(ittValue);
						std::string i_help = m_pManager->GetValueHelp(ittValue);

						struct tm timeinfo;
						localtime_r(&ittCmds.second.m_LastSeen, &timeinfo);

						char szDate[30];
						strftime(szDate, sizeof(szDate), "%Y-%m-%d %H:%M:%S", &timeinfo);

						root["result"][index]["config"][ivalue]["index"] = i_index;
						root["result"][index]["config"][ivalue]["label"] = i_label;
						root["result"][index]["config"][ivalue]["units"] = i_units;
						root["result"][index]["config"][ivalue]["help"] = i_help;
						root["result"][index]["config"][ivalue]["LastUpdate"] = szDate;
						ivalue++;
					}
					else if (commandclass == COMMAND_CLASS_WAKE_UP)
					{
						//Only add the Wake_Up_Interval value here
						if ((ittValue.GetGenre() == OpenZWave::ValueID::ValueGenre_System) && (ittValue.GetInstance() == 1))
						{
							if ((m_pManager->GetValueLabel(ittValue) == "Wake-up Interval") && (ittValue.GetType() == OpenZWave::ValueID::ValueType_Int))
							{
								if (m_pManager->GetValueAsString(ittValue, &szValue) == false)
								{
									continue;
								}
								root["result"][index]["config"][ivalue]["type"] = "int";
								root["result"][index]["config"][ivalue]["value"] = szValue;
								int i_index = 2000 + ittValue.GetIndex(); //special case
								std::string i_label = m_pManager->GetValueLabel(ittValue);
								std::string i_units = m_pManager->GetValueUnits(ittValue);
								std::string i_help = m_pManager->GetValueHelp(ittValue);

								struct tm timeinfo;
								localtime_r(&ittCmds.second.m_LastSeen, &timeinfo);

								char szDate[30];
								strftime(szDate, sizeof(szDate), "%Y-%m-%d %H:%M:%S", &timeinfo);
								root["result"][index]["config"][ivalue]["index"] = i_index;
								root["result"][index]["config"][ivalue]["label"] = i_label;
								root["result"][index]["config"][ivalue]["units"] = i_units;
								root["result"][index]["config"][ivalue]["help"] = i_help;
								root["result"][index]["config"][ivalue]["LastUpdate"] = szDate;
								ivalue++;
							}
						}
					}
				}
			}
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
}

bool COpenZWave::ApplyNodeConfig(const unsigned int homeID, const int nodeID, const std::string& svaluelist)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == NULL)
			return false;

		std::vector<std::string> results;
		StringSplit(svaluelist, "_", results);
		if (results.size() % 2 != 0)
			return false;

		bool bRestartOpenZWave = false;

		size_t vindex = 0;
		while (vindex < results.size())
		{
			int rvIndex = atoi(results[vindex].c_str());
			std::string ValueVal = results[vindex + 1];
			ValueVal = base64_decode(ValueVal);

			if (nodeID == m_controllerNodeId)
			{
				//Main ZWave node (Controller)
				if (rvIndex == 1)
				{
					//PollInterval
					int intervalseconds = atoi(ValueVal.c_str());
					m_sql.UpdatePreferencesVar("ZWavePollInterval", intervalseconds);
					EnableDisableNodePolling(nodeID);
				}
				else if (rvIndex == 2)
				{
					//Debug mode
					int debugenabled = (ValueVal == "Disabled") ? 0 : 1;
					int old_debugenabled = 0;
					m_sql.GetPreferencesVar("ZWaveEnableDebug", old_debugenabled);
					if (old_debugenabled != debugenabled)
					{
						m_sql.UpdatePreferencesVar("ZWaveEnableDebug", debugenabled);
						bRestartOpenZWave = true;
					}
				}
				else if (rvIndex == 3)
				{
					//Nightly Node Heal
					int nightly_heal = (ValueVal == "Disabled") ? 0 : 1;
					int old_nightly_heal = 0;
					m_sql.GetPreferencesVar("ZWaveEnableNightlyNetworkHeal", old_nightly_heal);
					if (old_nightly_heal != nightly_heal)
					{
						m_sql.UpdatePreferencesVar("ZWaveEnableNightlyNetworkHeal", nightly_heal);
						m_bNightlyNetworkHeal = (nightly_heal != 0);
					}
				}
				else if (rvIndex == 4)
				{
					//Security Key
					std::string networkkey = ValueVal;
					std::string old_networkkey = "";
					m_sql.GetPreferencesVar("ZWaveNetworkKey", old_networkkey);
					if (old_networkkey != networkkey)
					{
						std::vector<std::string> splitresults;
						StringSplit(networkkey, ",", splitresults);
						if (splitresults.size() != 16)
						{
							networkkey = "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10";
						}
						if (old_networkkey != networkkey)
						{
							m_sql.UpdatePreferencesVar("ZWaveNetworkKey", networkkey.c_str());
							bRestartOpenZWave = true;
						}
					}
				}
				else if (rvIndex == 5)
				{
					//RetryTimeout
					int nValue = atoi(ValueVal.c_str());
					int old_value = 3000;
					m_sql.GetPreferencesVar("ZWaveRetryTimeout", old_value);
					if (old_value != nValue)
					{
						m_sql.UpdatePreferencesVar("ZWaveRetryTimeout", nValue);
						bRestartOpenZWave = true;
					}
				}
				else if (rvIndex == 6)
				{
					//AssumeAwake
					int nValue = (ValueVal == "False") ? 0 : 1;
					int old_value = 0;
					m_sql.GetPreferencesVar("ZWaveAssumeAwake", old_value);
					if (old_value != nValue)
					{
						m_sql.UpdatePreferencesVar("ZWaveAssumeAwake", nValue);
						bRestartOpenZWave = true;
					}
				}
				else if (rvIndex == 7)
				{
					//PerformReturnRoutes
					int nValue = (ValueVal == "False") ? 0 : 1;
					int old_value = 1;
					m_sql.GetPreferencesVar("ZWavePerformReturnRoutes", old_value);
					if (old_value != nValue)
					{
						m_sql.UpdatePreferencesVar("ZWavePerformReturnRoutes", nValue);
						bRestartOpenZWave = true;
					}
				}
				else if (rvIndex == 8)
				{
					uint16_t Manufacturer_id = GetUInt16FromString(m_pManager->GetNodeManufacturerId(m_controllerID, m_controllerNodeId));
					uint16_t Product_type = GetUInt16FromString(m_pManager->GetNodeProductType(m_controllerID, m_controllerNodeId));
					uint16_t Product_id = GetUInt16FromString(m_pManager->GetNodeProductId(m_controllerID, m_controllerNodeId));

					if (Manufacturer_id == 0x0086)
					{
						if (
							((Product_type == 0x0001) && (Product_id == 0x005a)) || //Z-Stick Gen5
							((Product_type == 0x0001) && (Product_id == 0x005c)) || //Z-Stick Lite Gen5
							((Product_type == 0x0101) && (Product_id == 0x005a)) || //Z-Stick Gen5
							((Product_type == 0x0201) && (Product_id == 0x005a))    //Z-Stick Gen5
							)
						{
							int blinkenabled = (ValueVal == "Disabled") ? 0 : 1;
							int old_blinkenabled = 1;
							m_sql.GetPreferencesVar("ZWaveAeotecBlinkEnabled", old_blinkenabled);
							if (old_blinkenabled != blinkenabled)
							{
								m_sql.UpdatePreferencesVar("ZWaveAeotecBlinkEnabled", blinkenabled);

								CloseSerialConnector();

								serial::Serial _serial;
								_serial.setPort(m_szSerialPort);
								_serial.setBaudrate(115200);
								_serial.setBytesize(serial::eightbits);
								_serial.setParity(serial::parity_none);
								_serial.setStopbits(serial::stopbits_one);
								_serial.setFlowcontrol(serial::flowcontrol_none);
								serial::Timeout stimeout = serial::Timeout::simpleTimeout(100);
								_serial.setTimeout(stimeout);
								try
								{
									_serial.open();

									uint8_t _AeotecBlink_On[10] = { 0x01, 0x08, 0x00, 0xF2, 0x51, 0x01, 0x01, 0x05, 0x01, 0x50 };
									uint8_t _AeotecBlink_Off[10] = { 0x01, 0x08, 0x00, 0xF2, 0x51, 0x01, 0x00, 0x05, 0x01, 0x51 };

									if (blinkenabled == 1)
										_serial.write(_AeotecBlink_On, 10);
									else
										_serial.write(_AeotecBlink_Off, 10);

									_serial.close();
								}
								catch (...)
								{
									_log.Log(LOG_ERROR, "OpenZWave: Problem with setting Aeotec's Controller blinking mode!");
								}

								bRestartOpenZWave = true;
								m_bAeotecBlinkingMode = true;
							}
						}
					}
				}
			}
			else
			{
				OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
				if (GetNodeConfigValueByIndex(pNode, rvIndex, vID))
				{
					std::string vstring;
					m_pManager->GetValueAsString(vID, &vstring);

					OpenZWave::ValueID::ValueType vType = vID.GetType();

					if (vstring != ValueVal)
					{
						bool bRet = false;
						if (vType == OpenZWave::ValueID::ValueType_List)
						{
							bRet = m_pManager->SetValueListSelection(vID, ValueVal);
						}
						else if (vType == OpenZWave::ValueID::ValueType_String)
						{
							bRet = m_pManager->SetValue(vID, ValueVal);
						}
						else if (vType == OpenZWave::ValueID::ValueType_Int)
						{
							bRet = m_pManager->SetValue(vID, atoi(ValueVal.c_str()));
						}
						else if (vType == OpenZWave::ValueID::ValueType_BitSet)
						{
							bRet = m_pManager->SetValue(vID, atoi(ValueVal.c_str()));
						}
						else if (vType == OpenZWave::ValueID::ValueType_Byte)
						{
							bRet = m_pManager->SetValue(vID, (uint8)atoi(ValueVal.c_str()));
						}
						else if (vType == OpenZWave::ValueID::ValueType_Short)
						{
							bRet = m_pManager->SetValue(vID, (int16)atoi(ValueVal.c_str()));
						}
						else if (vType == OpenZWave::ValueID::ValueType_Decimal)
						{
							bRet = m_pManager->SetValue(vID, (float)atof(ValueVal.c_str()));
						}
						else
						{
							_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
							return false;
						}
						if (!bRet)
						{
							std::string cvLabel = m_pManager->GetValueLabel(vID);
							_log.Log(LOG_ERROR, "OpenZWave: Error setting value: %s (%s)", cvLabel.c_str(), ValueVal.c_str());
							return false;
						}

					}
				}
			}
			vindex += 2;
		}

		if (bRestartOpenZWave)
		{
			//Restart
			OpenSerialConnector();
		}
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

//User Code routines
bool COpenZWave::SetUserCodeEnrollmentMode()
{
	m_ControllerCommandStartTime = mytime(NULL);
	m_bControllerCommandInProgress = true;
	m_bControllerCommandCanceled = false;
	m_bInUserCodeEnrollmentMode = true;
	_log.Log(LOG_STATUS, "OpenZWave: User Code Enrollment mode initiated...");
	return false;
}

bool COpenZWave::GetNodeUserCodes(const unsigned int homeID, const int nodeID, Json::Value& root)
{
	try
	{
		int ii = 0;
		COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (!pNode)
			return false;
		if (pNode->Instances.find(1) == pNode->Instances.end())
			return false; //no codes added yet, wake your tag reader

		for (std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.begin(); itt != pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.end(); ++itt)
		{
			OpenZWave::ValueID vNode = *itt;
			if ((vNode.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (vNode.GetInstance() == 1) && (vNode.GetType() == OpenZWave::ValueID::ValueType_Raw))
			{
				int vNodeIndex = vNode.GetIndex();
				if (vNodeIndex >= 1)
				{
					if (vNode.GetType() == OpenZWave::ValueID::ValueType_String)
					{
						std::string sValue;
						if (m_pManager->GetValueAsString(vNode, &sValue))
						{
							if (sValue.find("0x00 ") != 0)
							{
								root["result"][ii]["index"] = vNodeIndex;
								root["result"][ii]["code"] = sValue;
								ii++;
							}
						}
					}
					else
					{
						_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vNode.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
						return false;
					}
				}
			}
		}
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
	}
	return false;
}

bool COpenZWave::RemoveUserCode(const unsigned int homeID, const int nodeID, const int codeIndex)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (!pNode)
		return false;
	if (pNode->Instances.find(1) == pNode->Instances.end())
		return false; //no codes added yet, wake your tag reader

	for (std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.begin(); itt != pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values.end(); ++itt)
	{
		OpenZWave::ValueID vNode = *itt;
		if ((vNode.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (vNode.GetInstance() == 1) && (vNode.GetType() == OpenZWave::ValueID::ValueType_Raw))
		{
			int vNodeIndex = vNode.GetIndex();
			if (vNodeIndex == codeIndex)
			{
				try
				{
					if (vNode.GetType() == OpenZWave::ValueID::ValueType_String)
					{
						std::string sValue;
						if (m_pManager->GetValueAsString(vNode, &sValue))
						{
							//Set our code to zero
							//First find code length in bytes
							int cLength = (sValue.size() + 1) / 5;
							//Make an string with zero's
							sValue = "";
							for (int ii = 0; ii < cLength; ii++)
							{
								if (ii != cLength - 1)
									sValue += "0x00 ";
								else
									sValue += "0x00"; //last one
							}
							//Set the new (empty) code
							m_pManager->SetValue(vNode, sValue);
							break;
						}
					}
					else
					{
						_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d", vNode.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
						return false;
					}
				}
				catch (OpenZWave::OZWException& ex)
				{
					_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d)", ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine());
					return false;
				}
			}
		}
	}
	return true;
}

unsigned int COpenZWave::GetControllerID()
{
	return m_controllerID;
}

void COpenZWave::NightlyNodeHeal()
{
	if (!m_bNightlyNetworkHeal)
		return; //not enabled
	HealNetwork();
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_OpenZWaveNodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			m_ZW_Hwidx = iHardwareID;
			COpenZWave* pOZWHardware = (COpenZWave*)pHardware;

			root["status"] = "OK";
			root["title"] = "OpenZWaveNodes";

			root["NodesQueried"] = (pOZWHardware->m_awakeNodesQueried) || (pOZWHardware->m_allNodesQueried);
			root["ownNodeId"] = pOZWHardware->m_controllerNodeId;

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,HomeID,NodeID,Name,ProductDescription,PollTime FROM ZWaveNodes WHERE (HardwareID==%d)",
				iHardwareID);
			if (!result.empty())
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;

					unsigned int homeID = static_cast<unsigned int>(std::stoul(sd[1]));
					uint8 nodeID = (uint8)atoi(sd[2].c_str());
					//if (nodeID>1) //Don't include the controller
					{
						COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						root["result"][ii]["idx"] = sd[0];
						root["result"][ii]["HomeID"] = homeID;
						root["result"][ii]["NodeID"] = nodeID;
						root["result"][ii]["Name"] = sd[3];
						root["result"][ii]["Description"] = sd[4];
						root["result"][ii]["PollEnabled"] = (atoi(sd[5].c_str()) == 1) ? "true" : "false";
						root["result"][ii]["Version"] = pNode->iVersion;
						root["result"][ii]["Manufacturer_name"] = pNode->Manufacturer_name;

						root["result"][ii]["Manufacturer_id"] = int_to_hex(pNode->Manufacturer_id);
						root["result"][ii]["Product_type"] = int_to_hex(pNode->Product_type);
						root["result"][ii]["Product_id"] = int_to_hex(pNode->Product_id);
						root["result"][ii]["Product_name"] = pNode->Product_name;
						root["result"][ii]["State"] = pOZWHardware->GetNodeStateString(homeID, nodeID);
						root["result"][ii]["HaveUserCodes"] = pNode->HaveUserCodes;
						root["result"][ii]["IsPlus"] = pNode->IsPlus;
						root["result"][ii]["Generic_type"] = pOZWHardware->GetNodeGenericType(pNode->IsPlus, homeID, nodeID);

						char szDate[80];
						struct tm loctime;
						localtime_r(&pNode->LastSeen, &loctime);
						strftime(szDate, 80, "%Y-%m-%d %X", &loctime);

						root["result"][ii]["LastUpdate"] = szDate;

						//Add configuration parameters here
						pOZWHardware->GetNodeValuesJson(homeID, nodeID, root, ii);
						ii++;
					}
				}
			}
		}

		void CWebServer::Cmd_ZWaveUpdateNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::string name = request::findValue(&req, "name");
			std::string senablepolling = request::findValue(&req, "EnablePolling");
			if (
				(name == "") ||
				(senablepolling == "")
				)
				return;
			root["status"] = "OK";
			root["title"] = "UpdateZWaveNode";

			std::vector<std::vector<std::string> > result;

			m_sql.safe_query(
				"UPDATE ZWaveNodes SET Name='%q', PollTime=%d WHERE (ID=='%q')",
				name.c_str(),
				(senablepolling == "true") ? 1 : 0,
				idx.c_str()
			);
			result = m_sql.safe_query("SELECT HardwareID, HomeID, NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->SetNodeName(homeID, nodeID, name);
					pOZWHardware->EnableDisableNodePolling(nodeID);
				}
			}
		}

		void CWebServer::Cmd_ZWaveDeleteNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RemoveFailedDevice(nodeID);
					root["status"] = "OK";
					root["title"] = "DeleteZWaveNode";
					result = m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (ID=='%q')", idx.c_str());
				}
			}

		}

		void CWebServer::Cmd_ZWaveInclude(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::string ssecure = request::findValue(&req, "secure");
			bool bSecure = (ssecure == "true");
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				m_sql.AllowNewHardwareTimer(5);
				pOZWHardware->IncludeDevice(bSecure);
				root["status"] = "OK";
				root["title"] = "ZWaveInclude";
			}
		}

		void CWebServer::Cmd_ZWaveExclude(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ExcludeDevice(1);
				root["status"] = "OK";
				root["title"] = "ZWaveExclude";
			}
		}

		void CWebServer::Cmd_ZWaveIsNodeIncluded(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				root["status"] = "OK";
				root["title"] = "ZWaveIsNodeIncluded";
				bool bIsIncluded = pOZWHardware->IsNodeIncluded();
				root["result"] = bIsIncluded;
				if (bIsIncluded)
				{
					root["node_id"] = pOZWHardware->m_LastIncludedNode;
					root["node_type"] = pOZWHardware->m_LastIncludedNodeType;
					std::string productName("Unknown");
					COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(pOZWHardware->GetControllerID(), pOZWHardware->m_LastIncludedNode);
					if (pNode)
					{
						productName = pNode->Product_name;
					}
					root["node_product_name"] = productName;
				}
			}
		}

		void CWebServer::Cmd_ZWaveIsNodeExcluded(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				root["status"] = "OK";
				root["title"] = "ZWaveIsNodeExcluded";
				root["result"] = pOZWHardware->IsNodeExcluded();
				root["node_id"] = pOZWHardware->m_LastRemovedNode;
			}
		}

		void CWebServer::Cmd_ZWaveSoftReset(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SoftResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveSoftReset";
			}
		}

		void CWebServer::Cmd_ZWaveHardReset(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HardResetDevice();
				root["status"] = "OK";
				root["title"] = "ZWaveHardReset";
			}
		}

		void CWebServer::Cmd_ZWaveStateCheck(WebEmSession& session, const request& req, Json::Value& root)
		{
			root["title"] = "ZWaveStateCheck";
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				if (!pOZWHardware->GetFailedState()) {
					root["status"] = "OK";
				}
			}
			return;
		}

		void CWebServer::Cmd_ZWaveNetworkHeal(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNetwork();
				root["status"] = "OK";
				root["title"] = "ZWaveHealNetwork";
			}
		}

		void CWebServer::Cmd_ZWaveNodeHeal(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::string node = request::findValue(&req, "node");
			if (node == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->HealNode(atoi(node.c_str()));
				root["status"] = "OK";
				root["title"] = "ZWaveHealNode";
			}
		}

		void CWebServer::Cmd_ZWaveNetworkInfo(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			root["title"] = "ZWaveNetworkInfo";

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			int hwID = atoi(idx.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwID);
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::vector< std::vector< int > > nodevectors;

				if (pOZWHardware->NetworkInfo(hwID, nodevectors)) {

					std::vector<std::vector<int> >::iterator row_iterator;
					std::vector<int>::iterator col_iterator;

					std::vector<int> allnodes;
					int rowCount = 0;
					std::stringstream allnodeslist;
					for (row_iterator = nodevectors.begin(); row_iterator != nodevectors.end(); ++row_iterator) {
						int colCount = 0;
						int nodeID = -1;
						std::vector<int> rest;
						std::stringstream list;

						for (col_iterator = (*row_iterator).begin(); col_iterator != (*row_iterator).end(); ++col_iterator) {
							if (colCount == 0) {
								nodeID = *col_iterator;
							}
							else {
								rest.push_back(*col_iterator);
							}
							colCount++;
						}

						if (nodeID != -1)
						{
							std::copy(rest.begin(), rest.end(), std::ostream_iterator<int>(list, ","));
							root["result"]["mesh"][rowCount]["nodeID"] = nodeID;
							allnodes.push_back(nodeID);
							root["result"]["mesh"][rowCount]["seesNodes"] = list.str();
							rowCount++;
						}
					}
					std::copy(allnodes.begin(), allnodes.end(), std::ostream_iterator<int>(allnodeslist, ","));
					root["result"]["nodes"] = allnodeslist.str();
					root["status"] = "OK";
				}

			}
		}

		void CWebServer::Cmd_ZWaveRemoveGroupNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::string node = request::findValue(&req, "node");
			if (node == "")
				return;
			std::string group = request::findValue(&req, "group");
			if (group == "")
				return;
			std::string removenode = request::findValue(&req, "removenode");
			if (removenode == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				int nodeId = 0, instance = 0;
				sscanf(removenode.c_str(), "%d.%d", &nodeId, &instance);
				pOZWHardware->RemoveNodeFromGroup(atoi(node.c_str()), atoi(group.c_str()), nodeId, instance);
				root["status"] = "OK";
				root["title"] = "ZWaveRemoveGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveAddGroupNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::string node = request::findValue(&req, "node");
			if (node == "")
				return;
			std::string group = request::findValue(&req, "group");
			if (group == "")
				return;
			std::string addnode = request::findValue(&req, "addnode");
			if (addnode == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				int nodeId = 0, instance = 0;
				sscanf(addnode.c_str(), "%d.%d", &nodeId, &instance);
				pOZWHardware->AddNodeToGroup(atoi(node.c_str()), atoi(group.c_str()), nodeId, instance);
				root["status"] = "OK";
				root["title"] = "ZWaveAddGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveGroupInfo(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			int iHardwareID = atoi(idx.c_str());

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID,HomeID,NodeID,Name FROM ZWaveNodes WHERE (HardwareID==%d)",
					iHardwareID);

				if (!result.empty())
				{
					int MaxNoOfGroups = 0;
					std::vector<std::vector<std::string> >::const_iterator itt;
					int ii = 0;
					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
						unsigned int homeID = static_cast<unsigned int>(std::stoul(sd[1]));
						int nodeID = atoi(sd[2].c_str());
						COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == NULL)
							continue;
						std::string nodeName = sd[3].c_str();
						int numGroups = pOZWHardware->ListGroupsForNode(nodeID);
						root["result"]["nodes"][ii]["nodeID"] = nodeID;
						root["result"]["nodes"][ii]["nodeName"] = nodeName;
						root["result"]["nodes"][ii]["groupCount"] = numGroups;
						if (numGroups > 0) {
							if (numGroups > MaxNoOfGroups)
								MaxNoOfGroups = numGroups;

							std::vector< std::string > nodesingroup;
							int gi = 0;
							for (int x = 1; x <= numGroups; x++)
							{
								int numNodesInGroup = pOZWHardware->ListAssociatedNodesinGroup(nodeID, x, nodesingroup);
								if (numNodesInGroup > 0) {
									std::stringstream list;
									std::copy(nodesingroup.begin(), nodesingroup.end(), std::ostream_iterator<std::string>(list, ","));
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["groupName"] = pOZWHardware->GetGroupName(nodeID, x);
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = list.str();
								}
								else {
									root["result"]["nodes"][ii]["groups"][gi]["id"] = x;
									root["result"]["nodes"][ii]["groups"][gi]["groupName"] = pOZWHardware->GetGroupName(nodeID, x);
									root["result"]["nodes"][ii]["groups"][gi]["nodes"] = "";
								}
								gi++;
								nodesingroup.clear();
							}

						}
						ii++;
					}
					root["result"]["MaxNoOfGroups"] = MaxNoOfGroups;

				}
			}
			root["status"] = "OK";
			root["title"] = "ZWaveGroupInfo";
		}

		void CWebServer::Cmd_ZWaveCancel(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->CancelControllerCommand(true);
				root["status"] = "OK";
				root["title"] = "ZWaveCancel";
			}
		}

		void CWebServer::Cmd_ApplyZWaveNodeConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string svaluelist = request::findValue(&req, "valuelist");
			if (
				(idx == "") ||
				(svaluelist == "")
				)
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->ApplyNodeConfig(homeID, nodeID, svaluelist))
						return;
					root["status"] = "OK";
					root["title"] = "ApplyZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveRequestNodeConfig(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeConfig(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeConfig";
				}
			}
		}
		void CWebServer::Cmd_ZWaveRequestNodeInfo(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeInfo(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeInfo";
				}
			}
		}

		void CWebServer::Cmd_ZWaveReceiveConfigurationFromOtherController(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->ReceiveConfigurationFromOtherController();
				root["status"] = "OK";
				root["title"] = "ZWaveReceiveConfigurationFromOtherController";
			}
		}

		void CWebServer::Cmd_ZWaveSendConfigurationToSecondaryController(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->SendConfigurationToSecondaryController();
				root["status"] = "OK";
				root["title"] = "ZWaveSendConfigToSecond";
			}
		}

		void CWebServer::Cmd_ZWaveTransferPrimaryRole(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->TransferPrimaryRole();
				root["status"] = "OK";
				root["title"] = "ZWaveTransferPrimaryRole";
			}
		}
		void CWebServer::ZWaveGetConfigFile(WebEmSession& session, const request& req, reply& rep)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::string configFilePath = "";
					pOZWHardware->GetConfigFile(configFilePath, rep.content);
					if (!configFilePath.empty() && !rep.content.empty()) {
						std::string filename;
						std::size_t last_slash_pos = configFilePath.find_last_of("/");
						if (last_slash_pos != std::string::npos) {
							filename = configFilePath.substr(last_slash_pos + 1);
						}
						else {
							filename = configFilePath;
						}
						reply::add_header_attachment(&rep, filename);
					}
				}
			}
		}
		void CWebServer::ZWaveCPIndex(WebEmSession& session, const request& req, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->m_ozwcp.SetAllNodesChanged();
					std::string wwwFile = szWWWFolder + "/ozwcp/cp.html";
					reply::set_content_from_file(&rep, wwwFile);
				}
			}
		}
		void CWebServer::ZWaveCPPollXml(WebEmSession& session, const request& req, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);

					reply::set_content(&rep, pOZWHardware->m_ozwcp.SendPollResponse());
					reply::add_header_attachment(&rep, "poll.xml");
				}
			}
		}
		void CWebServer::ZWaveCPNodeGetConf(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("node") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sNode = request::findValue(&values, "node");
			if (sNode == "")
				return;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.SendNodeConfResponse(iNode));
				}
			}
		}
		void CWebServer::ZWaveCPNodeGetValues(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("node") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sNode = request::findValue(&values, "node");
			if (sNode == "")
				return;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.SendNodeValuesResponse(iNode));
				}
			}
		}
		void CWebServer::ZWaveCPNodeSetValue(WebEmSession& session, const request& req, reply& rep)
		{
			std::vector<std::string> strarray;
			StringSplit(req.content, "=", strarray);
			if (strarray.size() != 2)
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.SetNodeValue(strarray[0], strarray[1]));
				}
			}
		}
		void CWebServer::ZWaveCPNodeSetButton(WebEmSession& session, const request& req, reply& rep)
		{
			std::vector<std::string> strarray;
			StringSplit(req.content, "=", strarray);
			if (strarray.size() != 2)
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.SetNodeButton(strarray[0], strarray[1]));
				}
			}
		}
		void CWebServer::ZWaveCPAdminCommand(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("fun") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sFun = request::findValue(&values, "fun");
			std::string sNode = request::findValue(&values, "node");
			std::string sButton = request::findValue(&values, "button");

			if (sNode.find("node") != std::string::npos)
			{
				sNode = sNode.substr(4);
			}

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.DoAdminCommand(sFun, atoi(sNode.c_str()), atoi(sButton.c_str())));
				}
			}
		}
		void CWebServer::ZWaveCPNodeChange(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("fun") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sFun = request::findValue(&values, "fun");
			std::string sNode = request::findValue(&values, "node");
			std::string sValue = request::findValue(&values, "value");

			if (sNode.size() > 4)
				sNode = sNode.substr(4);

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.DoNodeChange(sFun, atoi(sNode.c_str()), sValue));
				}
			}
		}
		void CWebServer::ZWaveCPSetGroup(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("fun") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sFun = request::findValue(&values, "fun");
			std::string sNode = request::findValue(&values, "node");
			std::string sGroup = request::findValue(&values, "num");
			std::string glist = request::findValue(&values, "groups");

			if (sNode.size() > 4)
				sNode = sNode.substr(4);

			if (!sNode.empty() && !sGroup.empty() && !glist.empty())
			{
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
				if (pHardware != NULL)
				{
					if (pHardware->HwdType == HTYPE_OpenZWave)
					{
						COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
						std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
						reply::set_content(&rep, pOZWHardware->m_ozwcp.UpdateGroup(sFun, atoi(sNode.c_str()), atoi(sGroup.c_str()), glist));
					}
				}
			}
		}

		void CWebServer::ZWaveCPSaveConfig(WebEmSession& session, const request& req, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.SaveConfig());
				}
			}
		}
		void CWebServer::ZWaveCPGetTopo(WebEmSession& session, const request& req, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.GetCPTopo());
					reply::add_header_attachment(&rep, "topo.xml");
				}
			}
		}
		void CWebServer::ZWaveCPGetStats(WebEmSession& session, const request& req, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.GetCPStats());
					reply::add_header_attachment(&rep, "stats.xml");
				}
			}
		}
		void CWebServer::ZWaveCPSceneCommand(WebEmSession& session, const request& req, reply& rep)
		{
			if (req.content.find("fun") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sArg1 = request::findValue(&values, "fun");
			std::string sArg2 = request::findValue(&values, "id");
			std::string sVid = request::findValue(&values, "vid");
			std::string sLabel = request::findValue(&values, "label");
			std::string sArg3 = (!sVid.empty()) ? sVid : sLabel;
			std::string sArg4 = request::findValue(&values, "value");
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != NULL)
			{
				if (pHardware->HwdType == HTYPE_OpenZWave)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.DoSceneCommand(sArg1, sArg2, sArg3, sArg4));
				}
			}
		}

		void CWebServer::ZWaveCPTestHeal(WebEmSession& session, const request& req, reply& rep)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			if (req.content.find("fun") == std::string::npos)
				return;

			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sFun = request::findValue(&values, "fun");

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware == NULL)
				return;
			COpenZWave* pOZWHardware = (COpenZWave*)pHardware;

			if (pHardware->HwdType == HTYPE_OpenZWave)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);

				if (sFun == "test")
				{
					std::string sNum = request::findValue(&values, "num");
					std::string sArg = request::findValue(&values, "cnt");

					int node = atoi(sNum.c_str());
					int cnt = atoi(sArg.c_str());

					reply::set_content(&rep, pOZWHardware->m_ozwcp.DoTestNetwork(node, cnt));
					reply::add_header_attachment(&rep, "testheal.xml");
				}
				else if (sFun == "heal")
				{
					std::string sNum = request::findValue(&values, "num");
					std::string sHealRRS = request::findValue(&values, "healrrs");

					int node = atoi(sNum.c_str());
					bool healrrs = !sHealRRS.empty();

					reply::set_content(&rep, pOZWHardware->m_ozwcp.HealNetworkNode(node, healrrs));
					reply::add_header_attachment(&rep, "testheal.xml");
				}
			}
		}

		void CWebServer::Cmd_ZWaveSetUserCodeEnrollmentMode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != NULL)
			{
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				root["status"] = "OK";
				root["title"] = "SetUserCodeEnrollmentMode";
				pOZWHardware->SetUserCodeEnrollmentMode();
			}
		}

		void CWebServer::Cmd_ZWaveRemoveUserCode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			std::string scodeindex = request::findValue(&req, "codeindex");
			if (
				(idx == "") ||
				(scodeindex == "")
				)
				return;
			int iCodeIndex = atoi(scodeindex.c_str());

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->RemoveUserCode(homeID, nodeID, iCodeIndex))
						return;
					root["status"] = "OK";
					root["title"] = "RemoveUserCode";
				}
			}
		}

		void CWebServer::Cmd_ZWaveGetNodeUserCodes(WebEmSession& session, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				int nodeID = atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != NULL)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->GetNodeUserCodes(homeID, nodeID, root))
						return;
					root["status"] = "OK";
					root["title"] = "GetUserCodes";
				}
			}
		}
	}
}

#endif //WITH_OPENZWAVE
