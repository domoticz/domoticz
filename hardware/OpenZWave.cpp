#include "stdafx.h"
#ifdef WITH_OPENZWAVE
#include "OpenZWave.h"

#include <sstream>      // std::stringstream
#include <ctype.h>
#include <algorithm>

#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../webserver/Base64.h"
#include "hardwaretypes.h"

#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/mainworker.h"

#include <json/json.h>
#include "../main/localtime_r.h"

//OpenZWave includes
#include <Options.h>
#include <Manager.h>
#include <platform/Log.h>
#include <ValueIDIndexesDefines.h>

#include "ZWaveCommands.h"

#include "serial/serial.h"

extern std::string szWWWFolder;

//Note!, Some devices uses the same instance for multiple values,
//to solve this we are going to use the Index value!, Except for COMMAND_CLASS_MULTI_INSTANCE
//This is for backwards compatability .... but should not be used
namespace
{
	constexpr std::array<std::pair<const char *, uint8_t>, 23> AlarmToIndexMapping{
		{
			{ "General", 0x28 },	      //
			{ "Smoke", 0x29 },	      //
			{ "Carbon Monoxide", 0x2A },  //
			{ "Carbon Dioxide", 0x2B },   //
			{ "Heat", 0x2C },	      //
			{ "Water", 0x2D },	      //
			{ "Flood", 0x2D },	      //
			{ "Alarm Level", 0x32 },      //
			{ "Alarm Type", 0x33 },	      //
			{ "Access Control", 0x34 },   //
			{ "Burglar", 0x35 },	      //
			{ "Home Security", 0x35 },    //
			{ "Power Management", 0x36 }, //
			{ "System", 0x37 },	      //
			{ "Emergency", 0x38 },	      //
			{ "Clock", 0x39 },	      //
			{ "Appliance", 0x3A },	      //
			{ "HomeHealth", 0x3B },	      //
			{ "Siren", 0x3C },	      //
			{ "Water Valve", 0x3D },      //
			{ "Weather", 0x3E },	      //
			{ "Irrigation", 0x3F },	      //
			{ "Gas", 0x40 },	      //
		}				      //
	};
} // namespace

uint8_t GetIndexFromAlarm(const std::string& sLabel)
{
	for (const auto &alarm : AlarmToIndexMapping)
	{
		if (sLabel.find(alarm.first) != std::string::npos)
			return alarm.second;
	}
	return 0;
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
	case 0x79:
		return "SOUND SWITCH";
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
	m_pManager = nullptr;
	m_bAeotecBlinkingMode = false;
}

COpenZWave::~COpenZWave()
{
	CloseSerialConnector();
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this home/node id
//-----------------------------------------------------------------------------
COpenZWave::NodeInfo* COpenZWave::GetNodeInfo(const unsigned int homeID, const uint8_t nodeID)
{
	for (auto &node : m_nodes)
		if ((node.homeId == homeID) && (node.nodeId == nodeID))
			return &node;

	return nullptr;
}

std::string COpenZWave::GetNodeStateString(const unsigned int homeID, const uint8_t nodeID)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d", 
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d", 
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
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
	// _log.Log(LOG_STATUS, "OpenZWave: Notification: %s (%d))",_notification->GetAsString().c_str(),_notification->GetNodeId());

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
	m_updateTime = mytime(nullptr);

	OpenZWave::ValueID vID = _notification->GetValueID();
	uint8_t commandClass = vID.GetCommandClassId();
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
		for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
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
			AddValue(nodeInfo, vID);
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueAdded, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_SceneEvent:
		//Deprecated
	{
		//uint8_t SceneID = _notification->GetSceneId();
		//_log.Log(LOG_NORM, "OpenZWave: Type_SceneEvent, SceneID: %d, HomeID: %u, NodeID: %d (0x%02x)", SceneID, _homeID, _nodeID, _nodeID);
	}
	break;
	case OpenZWave::Notification::Type_ValueRemoved:
		if ((_nodeID == 0) || (_nodeID == 255))
			return;
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			// Remove the value from out list
			for (auto it = nodeInfo->Instances[instance][commandClass].Values.begin();
			     it != nodeInfo->Instances[instance][commandClass].Values.end(); ++it)
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
		if (NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->eState = NSTATE_AWAKE;
			nodeInfo->LastSeen = m_updateTime;
			UpdateValue(nodeInfo, vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueChanged, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_ValueRefreshed:
		if ((_nodeID == 0) || (_nodeID == 255))
		{
			_log.Log(LOG_STATUS, "OpenZWave: Invalid NodeID received. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
			return;
		}
		if (NodeInfo* nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			nodeInfo->eState = NSTATE_AWAKE;
			nodeInfo->LastSeen = m_updateTime;
			UpdateValue(nodeInfo, vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen = m_updateTime;
		}
		else
			_log.Log(LOG_ERROR, "OpenZWave: Type_ValueRefreshed, NodeID not found internally!. HomeID: %u, NodeID: %d (0x%02x)", _homeID, _nodeID, _nodeID);
		break;
	case OpenZWave::Notification::Type_Notification:
	{
		NodeInfo* nodeInfo = GetNodeInfo(_homeID, _nodeID);
		if (nodeInfo == nullptr)
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
			// Check if node was replaced
			if (!m_bNodeReplaced) {
				// _log.Log(LOG_STATUS, "OpenZWave: Comparing node id (%d) with node te be replaced (%d)", _nodeID, m_NodeToBeReplaced);
				if (_nodeID==m_NodeToBeReplaced) {
					_log.Log(LOG_STATUS,"OpenZWave: Node replaced");
					m_bNodeReplaced=true;
				} 
			} else {
				_log.Log(LOG_STATUS,"OpenZWave: Node Alive");
			}

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
		// _log.Log(LOG_STATUS, "OpenZWave: Type_ControllerCommand %s", _notification->GetAsString().c_str());

		uint8_t controller_command = _notification->GetCommand();
		if (controller_command == OpenZWave::Driver::ControllerCommand_RemoveDevice)
		{
			uint8_t controler_state = _notification->GetEvent();
			if (controler_state == OpenZWave::Driver::ControllerState::ControllerState_Failed)
			{
				m_LastRemovedNode = -1;
			}
		}
		if (controller_command == OpenZWave::Driver::ControllerCommand_HasNodeFailed)
		{
			uint8_t controler_state = _notification->GetEvent();
			if (_nodeID==m_HasNodeFailedIdx) {
				if (controler_state == OpenZWave::Driver::ControllerState::ControllerState_NodeOK)
				{
					// _log.Log(LOG_STATUS, "OpenZWave: Event Node OK (%s)", _notification->GetAsString().c_str());
					m_bHasNodeFailedDone=true;
					m_sHasNodeFailedResult="Node OK";
				} 
				else if (controler_state == OpenZWave::Driver::ControllerState::ControllerState_NodeFailed)
				{
					// _log.Log(LOG_STATUS, "OpenZWave: Event Node failed (%s)", _notification->GetAsString().c_str());
					m_bHasNodeFailedDone=true;
					m_sHasNodeFailedResult="Node Failed";
				}
				else if (controler_state == OpenZWave::Driver::ControllerState::ControllerState_Failed)
				{
					// If this happens, some weird error, which should never happen.
					_log.Log(LOG_ERROR, "OpenZWave: Has Node Failed Command on Node %d failed (%s)", m_HasNodeFailedIdx, _notification->GetAsString().c_str());
					m_bHasNodeFailedDone=true;
					m_sHasNodeFailedResult = _notification->GetAsString();
				} else {
					// All other notifications from Has Node Failed Node Command
					// _log.Log(LOG_STATUS, "OpenZWave: Has Node Failed command Controller Comand (%s)", _notification->GetAsString().c_str());
				}
			}
		}
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
	case OpenZWave::Notification::Type_UserAlerts:
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
	m_updateTime = mytime(nullptr);
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
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it nullptr
	// the log file will appear in the program's working directory.
	_log.Log(LOG_STATUS, "OpenZWave: using config in: %s", ConfigPath.c_str());

	try
	{
		OpenZWave::Options::Create(ConfigPath, UserPath, "--SaveConfiguration=true ");
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d", 
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		return false;
	}

	EnableDisableDebug();
	OpenZWave::Options::Get()->AddOptionInt("PollInterval", 60000); //enable polling each 60 seconds
	OpenZWave::Options::Get()->AddOptionBool("IntervalBetweenPolls", true);
	OpenZWave::Options::Get()->AddOptionBool("ValidateValueChanges", true);
	OpenZWave::Options::Get()->AddOptionBool("Associate", true);

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

	nValue = 0; //default false
	m_sql.GetPreferencesVar("ZWaveAutoUpdateConfigFile", nValue);
	OpenZWave::Options::Get()->AddOptionBool("AutoUpdateConfigFile", (nValue == 1) ? true : false);
	OpenZWave::Options::Get()->AddOptionString("ReloadAfterUpdate", "NEVER", false);


	try
	{
		OpenZWave::Options::Get()->Lock();

		OpenZWave::Manager::Create();
		m_pManager = OpenZWave::Manager::Get();
		if (m_pManager == nullptr)
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
		if (!m_pManager->AddDriver(m_szSerialPort))
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
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
	m_pManager = nullptr;
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

bool COpenZWave::GetValueByCommandClass(const uint8_t nodeID, const uint8_t instanceID, const uint8_t commandClass, OpenZWave::ValueID& nValue)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
		return false;

	for (auto &value : pNode->Instances[instanceID][commandClass].Values)
	{
		uint8_t cmdClass = value.GetCommandClassId();
		if (cmdClass == commandClass)
		{
			std::string sLabel = m_pManager->GetValueLabel(value);
			if (m_pManager->IsValueReadOnly(value) == true)
				continue;
			OpenZWave::ValueID::ValueGenre vGenre = value.GetGenre();
			if (vGenre != OpenZWave::ValueID::ValueGenre_User)
				continue;
			nValue = value;
			return true;
		}
	}
	return false;
}

bool COpenZWave::GetValueByCommandClassIndex(const uint8_t nodeID, const uint8_t instanceID, const uint8_t commandClass, const uint16_t vIndex, OpenZWave::ValueID& nValue)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (!pNode)
		return false;

	for (auto &value : pNode->Instances[instanceID][commandClass].Values)
	{
		OpenZWave::ValueID::ValueGenre vGenre = value.GetGenre();
		uint8_t cmdClass = value.GetCommandClassId();
		if (cmdClass == commandClass)
		{
			if (m_pManager->IsValueReadOnly(value) == true)
				continue;
			if (vGenre != OpenZWave::ValueID::ValueGenre_User)
				continue;
			try
			{
				if (value.GetIndex() == vIndex)
				{
					nValue = value;
					return true;
				}
			}
			catch (OpenZWave::OZWException& ex)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
					ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
					std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return false;
			}
		}
	}
	return false;
}

bool COpenZWave::GetNodeConfigValueByIndex(const NodeInfo* pNode, const int index, OpenZWave::ValueID& nValue)
{
	for (const auto &ittInstance : pNode->Instances)
	{
		for (const auto &ittCmds : ittInstance.second)
		{
			for (const auto &value : ittCmds.second.Values)
			{
				uint16_t vindex = value.GetIndex();
				int vinstance = value.GetInstance();
				uint8_t commandclass = value.GetCommandClassId();
				if (
					(commandclass == COMMAND_CLASS_CONFIGURATION) &&
					(vindex == index)
					)
				{
					nValue = value;
					return true;
				}
				else if (
					(commandclass == COMMAND_CLASS_WAKE_UP) &&
					(vindex == index - 2000) //spacial case
					)
				{
					nValue = value;
					return true;
				}
				else if (
					(commandclass == COMMAND_CLASS_PROTECTION) &&
					(vinstance == index - 3000) //spacial case
					)
				{
					nValue = value;
					return true;
				}
			}
		}
	}
	return false;
}

bool COpenZWave::SwitchLight(_tZWaveDevice* pDevice, const int instanceID, const int value)
{
	if (m_pManager == nullptr)
		return false;
	if (pDevice == nullptr)
		return false;

	NodeInfo* pNode = GetNodeInfo(m_controllerID, pDevice->nodeID);
	if (!pNode)
	{
		if (!m_awakeNodesQueried)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Switch command not sent because not all Awake Nodes have been Queried!");
		}
		else {
			_log.Log(LOG_ERROR, "OpenZWave: Node not found! (NodeID: %d, 0x%02x)", pDevice->nodeID, pDevice->nodeID);
		}
		return false;
	}

	try
	{
		if (m_pManager->IsNodeFailed(m_controllerID, pDevice->nodeID))
		{
			_log.Log(LOG_ERROR, "OpenZWave: Node has failed (or is not alive), Switch command not sent! (NodeID: %d, 0x%02x)", pDevice->nodeID, pDevice->nodeID);
			return false;
		}

		if (
			((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0003))
			|| ((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0103))
			|| ((pDevice->Product_id == 0x0060) && (pDevice->Product_type == 0x0203))
			|| ((pDevice->Product_id == 0x00af) && (pDevice->Product_type == 0x0003))
			)
		{
			//Special case for the Aeotec Smart Switch
			if (pDevice->commandClassID == COMMAND_CLASS_SWITCH_MULTILEVEL)
			{
				pDevice = FindDevice(pDevice->nodeID, instanceID, COMMAND_CLASS_SWITCH_BINARY, ZWaveBase::ZDTYPE_SWITCH_NORMAL);
			}
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
			bool bFound = (GetValueByCommandClass(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_SWITCH_BINARY, vID) == true);
			if (!bFound)
				bFound = (GetValueByCommandClass(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_DOOR_LOCK, vID) == true);
			if (!bFound)
				bFound = (GetValueByCommandClassIndex(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL, ValueID_Index_SwitchMultiLevel::Level, vID) == true);
			if (!bFound)
				bFound = (GetValueByCommandClass(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_SOUND_SWITCH, vID) == true);
			if (bFound)
			{
				OpenZWave::ValueID::ValueType vType = vID.GetType();
				_log.Log(LOG_NORM, "OpenZWave: Domoticz has send a Switch command! NodeID: %d (0x%02x)", pDevice->nodeID, pDevice->nodeID);
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
						m_pManager->SetValue(vID, (uint8_t)0);
						pDevice->intvalue = 0;
					}
					else {
						//On
						m_pManager->SetValue(vID, (uint8_t)255);
						pDevice->intvalue = 255;
					}
				}
				else if (vType == OpenZWave::ValueID::ValueType_List)
				{
					std::vector<std::string > vStringList;
					m_pManager->GetValueListItems(vID, &vStringList);
					if (svalue == 255)
					{
						// Aeotec DoorBell 6 only
						if ((pDevice->Manufacturer_id == 0x0371) && (pDevice->Product_id == 0x00a2) && (pDevice->Product_type == 0x0003))
						{
							m_pManager->SetValueListSelection(vID, vStringList[31]); //default tone
							pDevice->intvalue = 255;
						}
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, pDevice->nodeID, pDevice->nodeID);
					return false;
				}

				if (svalue == 0)
				{
					//Device is switched off, lets see if there is a power sensor for this device,
					//and set its value to 0 as well
					_tZWaveDevice* pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
					if (pPowerDevice == nullptr)
					{
						pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, ZDTYPE_SENSOR_POWER);
					}
					if (pPowerDevice != nullptr)
					{
						pPowerDevice->floatValue = 0;
						SendDevice2Domoticz(pPowerDevice);
					}
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! NodeID: %d (0x%02x), instanceID: %d", pDevice->nodeID, pDevice->nodeID, instanceID);
				return false;
			}
		}
		else
		{
			//dimmable/color light  device
			if ((svalue > 99) && (svalue != 255))
				svalue = 99;
			if (GetValueByCommandClassIndex(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL, ValueID_Index_SwitchMultiLevel::Level, vID) == true)
			{
				std::string vLabel = m_pManager->GetValueLabel(vID);

				pDevice->intvalue = svalue;
				_log.Log(LOG_NORM, "OpenZWave: Domoticz has send a Switch command!, Level: %d, NodeID: %d (0x%02x)", svalue, pDevice->nodeID, pDevice->nodeID);

				if (vID.GetType() == OpenZWave::ValueID::ValueType_Byte)
				{
					if (!m_pManager->SetValue(vID, (uint8_t)svalue))
					{
						_log.Log(LOG_ERROR, "OpenZWave: Error setting Switch Value! NodeID: %d (0x%02x)", pDevice->nodeID, pDevice->nodeID);
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: SwitchLight, Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, pDevice->nodeID, pDevice->nodeID);
					return false;
				}

				if (svalue == 0)
				{
					//Device is switched off, lets see if there is a power sensor for this device,
					//and set its value to 0 as well
					_tZWaveDevice* pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, COMMAND_CLASS_METER, ZDTYPE_SENSOR_POWER);
					if (pPowerDevice == nullptr)
					{
						pPowerDevice = FindDevice(pDevice->nodeID, pDevice->instanceID, ZDTYPE_SENSOR_POWER);
					}
					if (pPowerDevice != nullptr)
					{
						pPowerDevice->floatValue = 0;
						SendDevice2Domoticz(pPowerDevice);
					}
				}

			}
			else if (GetValueByCommandClassIndex(pDevice->nodeID, (uint8_t)instanceID, COMMAND_CLASS_PROPRIETARY, ValueID_Index_SwitchMultiLevel::Level, vID) == true)
			{
				if (
					(pNode->Manufacturer_id == 0x010F)
					&& (pNode->Product_id == 0x1000)
					&& (pNode->Product_type == 0x0302)
					)
				{
					//Fibaro FGRM222 Roller Shutter Controller 2
					//Slat/Tilt position
					pDevice->intvalue = svalue;
					_log.Log(LOG_NORM, "OpenZWave: Domoticz has send a Slat/Tilt command!, Level: %d, NodeID: %d (0x%02x)", svalue, pDevice->nodeID, pDevice->nodeID);

					if (vID.GetType() == OpenZWave::ValueID::ValueType_Byte)
					{
						if (!m_pManager->SetValue(vID, (uint8_t)svalue))
						{
							_log.Log(LOG_ERROR, "OpenZWave: Error setting Slat/Tilt Value! NodeID: %d (0x%02x)", pDevice->nodeID, pDevice->nodeID);
						}
					}
					else
					{
						_log.Log(LOG_ERROR, "OpenZWave: SwitchLight, Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, pDevice->nodeID, pDevice->nodeID);
						return false;
					}
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! (NodeID: %d, 0x%02x)", pDevice->nodeID, pDevice->nodeID);
				return false;
			}
		}
		m_updateTime = mytime(nullptr);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	catch (...)
	{
	}
	return false;
}

bool COpenZWave::HasNodeFailed(const uint8_t nodeID)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::SwitchColor(const uint8_t nodeID, const uint8_t instanceID, const std::string& ColorStr)
{
	if (m_pManager == nullptr)
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
							unsigned wWhite = strtoul(OutColorStr.substr(7, 2).c_str(), nullptr, 16);
							unsigned cWhite = strtoul(OutColorStr.substr(9, 2).c_str(), nullptr, 16);
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
				_log.Log(LOG_ERROR, "OpenZWave: SwitchColor, Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, nodeID, nodeID);
				return false;
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Internal Node ValueID not found! (NodeID: %d, 0x%02x)", nodeID, nodeID);
			return false;
		}
		m_updateTime = mytime(nullptr);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	catch (...)
	{

	}
	return false;
}

void COpenZWave::SetThermostatSetPoint(const uint8_t nodeID, const uint8_t instanceID, const uint8_t /*commandClass*/, const float value)
{
	if (m_pManager == nullptr)
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
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vID.GetType(), std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, nodeID, nodeID);
				return;
			}
			m_updateTime = mytime(nullptr);
		}
		catch (OpenZWave::OZWException& ex)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
				ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
				std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		}
	}
}

void COpenZWave::DebugValue(const OpenZWave::ValueID& vID, const int Line)
{
	//unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();

	uint8_t vInstance = vID.GetInstance();//(See note on top of this file) GetInstance();
	uint16_t vIndex = vID.GetIndex();
	//std::string vLabel = m_pManager->GetValueLabel(vID);
	uint8_t commandclass = vID.GetCommandClassId();

	_log.Log(LOG_STATUS, "OpenZWave: Debug Value: Node: %d (0x%02x), CommandClass: %s, Instance: %d, Index: %d, Id: 0x%llX, Line: %d", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vInstance, vIndex, vID.GetId(), Line);
}


void COpenZWave::AddValue(NodeInfo* pNode, const OpenZWave::ValueID& vID)
{
	if (m_pManager == nullptr)
		return;
	if (!m_pManager->IsValueValid(vID))
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
	std::string vUnits = m_pManager->GetValueUnits(vID);

	if (vLabel == "Unknown") {
		_log.Log(LOG_STATUS, "OpenZWave: Value_Added: Warning: OpenZWave returned ValueLabel \"Unknown\" on Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex);
	}

	if (commandclass == COMMAND_CLASS_CONFIGURATION)
	{
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
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
				return;
			}
		}
	}

	//Ignore non-user types
	if (vGenre != OpenZWave::ValueID::ValueGenre_User)
		return;

	uint8_t instance = GetInstanceFromValueID(vID);

	_log.Log(LOG_NORM, "OpenZWave: Value_Added: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex);

	if ((instance == 0) && (NodeID == m_controllerID))
		return;// We skip instance 0 if there are more, since it should be mapped to other instances or their superposition

	_tZWaveDevice _device;
	_device.nodeID = NodeID;
	_device.instanceID = instance;
	_device.indexID = vIndex;
	_device.orgInstanceID = vOrgInstance;
	_device.orgIndexID = vOrgIndex;
	_device.commandClassID = commandclass;

	_device.label = vLabel;

	_device.hasWakeup = m_pManager->IsNodeAwake(HomeID, NodeID);
	_device.isListening = m_pManager->IsNodeListeningDevice(HomeID, NodeID);

	_device.Manufacturer_id = pNode->Manufacturer_id;
	_device.Product_id = pNode->Product_id;
	_device.Product_type = pNode->Product_type;

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
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
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
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
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
				_device.label = "Color";
				_device.devType = ZDTYPE_SWITCH_COLOR;
				_device.instanceID = 101;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_USER_CODE)
	{
		if ((vOrgIndex < ValueID_Index_UserCode::Code_1) || (vOrgIndex > ValueID_Index_UserCode::Code_253))
			return;
		pNode->HaveUserCodes = true;

		if (vType == OpenZWave::ValueID::ValueType_String)
		{
			std::string strValue;
			if (m_pManager->GetValueAsString(vID, &strValue) == true)
			{
			}
		}
		return;
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
		if (
			(vType != OpenZWave::ValueID::ValueType_List)
			&& (vType != OpenZWave::ValueID::ValueType_Byte)
			&& (vType != OpenZWave::ValueID::ValueType_String)
			)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
		if (vType == OpenZWave::ValueID::ValueType_String)
		{
			//could be a location or a user code that was entered, we do not use this for now
			std::string strValue;
			if (m_pManager->GetValueAsString(vID, &strValue) == true)
			{
			}
			return;
		}
		int32 alarm_value = 0;
		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			int32 listValue = 0;
			if (m_pManager->GetValueListSelection(vID, &listValue) == false)
				return;
			alarm_value = listValue;
		}
		else if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
				return;
			alarm_value = 0; //we assume a previous event cleared message was received
		}

		int newInstance = GetIndexFromAlarm(vLabel);
		if (newInstance == 0)
			newInstance = vOrgIndex;
		_device.instanceID = (uint8_t)newInstance;
		_device.devType = ZDTYPE_SWITCH_NORMAL;
		_device.intvalue = (alarm_value == 0) ? 0 : 255;
		InsertDevice(_device);
	}
	else if (commandclass == COMMAND_CLASS_METER)
	{
		if (
			(vOrgIndex == ValueID_Index_Meter::Exporting)
			|| (vLabel.find("Exporting") != std::string::npos)
			|| (vLabel.find("Interval") != std::string::npos)
			|| (vLabel.find("Previous Reading") != std::string::npos)
			)
			return; //Not interested
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
		if (m_pManager->GetValueAsFloat(vID, &fValue) == false)
		{
			return;
		}

		//Meter device
		if (
			(vOrgIndex == ValueID_Index_Meter::Electric_kWh)
			|| (vOrgIndex == ValueID_Index_Meter::Heating_kWh)
			|| (vOrgIndex == ValueID_Index_Meter::Cooling_kWh)
			)
		{
			_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
			_device.scaleMultiply = 1000.0f;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_kVah)
		{
			_device.devType = ZDTYPE_SENSOR_KVAH;
			_device.scaleMultiply = 1000.0f;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_kVar)
		{
			_device.devType = ZDTYPE_SENSOR_KVAR;
			_device.scaleMultiply = 1000.0f;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_kVarh)
		{
			_device.devType = ZDTYPE_SENSOR_KVARH;
			_device.scaleMultiply = 1000.0f;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_W)
		{
			_device.devType = ZDTYPE_SENSOR_POWER;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_V)
		{
			_device.devType = ZDTYPE_SENSOR_VOLTAGE;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_A)
		{
			_device.devType = ZDTYPE_SENSOR_AMPERE;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Electric_PowerFactor)
		{
			_device.devType = ZDTYPE_SENSOR_PERCENTAGE;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Gas_Cubic_Meters)
		{
			_device.devType = ZDTYPE_SENSOR_GAS;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Gas_Cubic_Feet)
		{
			_device.devType = ZDTYPE_SENSOR_GAS;
			_device.scaleMultiply = 0.0283168f; //(divide the volume value by 35,315)
		}
		else if (vOrgIndex == ValueID_Index_Meter::Water_Cubic_Meters)
		{
			_device.devType = ZDTYPE_SENSOR_WATER;
		}
		else if (vOrgIndex == ValueID_Index_Meter::Water_Cubic_Feet)
		{
			_device.devType = ZDTYPE_SENSOR_WATER;
			_device.scaleMultiply = 0.0283168f; //(divide the volume value by 35,315)
		}
		else if (vOrgIndex == ValueID_Index_Meter::Water_Cubic_US_Gallons)
		{
			_device.devType = ZDTYPE_SENSOR_WATER;
			_device.scaleMultiply = 0.00378541f; //(divide the volume value by 264, 172)
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled Meter type: %s, class: 0x%02X (%s), NodeID: %d (0x%02x), Index: %d, Instance: %d", vLabel.c_str(), commandclass, cclassStr(commandclass), NodeID, NodeID, vOrgIndex, vOrgInstance);
			return;
		}
		_device.floatValue = fValue * _device.scaleMultiply;
		InsertDevice(_device);
	}
	else if (commandclass == COMMAND_CLASS_SENSOR_MULTILEVEL)
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
		if (m_pManager->GetValueAsFloat(vID, &fValue) == false)
			return;

		if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::Air_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Water_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Soil_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Target_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Boiler_Water_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Domestic_Hot_Water_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Outside_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Exhaust_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Return_Air_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Supply_Air_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Condenser_Coil_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Evaporator_Coil_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Liquid_Line_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Discharge_Line_Temperature)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Defrost_Temperature)
			)
		{
			if (vUnits == "F")
			{
				//Convert to celcius
				fValue = float((fValue - 32) * (5.0 / 9.0));
			}
			_device.devType = ZDTYPE_SENSOR_TEMPERATURE;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Luminance)
		{
			// TODO decide what to do with sensors that report %.
			// - Do we know what 100% means? Is it always 1000 Lux?
			// - This comparison is pretty stable, but is it stable enough? (vUnits == "%")
			if (vUnits == "%")
			{
				//convert from % to Lux (where max is 1000 Lux)
				fValue = (1000.0f / 100.0f) * fValue;
				if (fValue > 1000.0f)
					fValue = 1000.0f;
			}
			_device.devType = ZDTYPE_SENSOR_LIGHT;
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::Humidity)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Soil_Humidity)
			)
		{
			_device.intvalue = round(fValue);
			_device.devType = ZDTYPE_SENSOR_HUMIDITY;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Ultraviolet)
		{
			_device.devType = ZDTYPE_SENSOR_UV;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Velocity)
		{
			_device.devType = ZDTYPE_SENSOR_VELOCITY;
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::Barometric_Pressure)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Atmospheric_Pressure)
			)
		{
			_device.scaleMultiply = 10.0f;
			_device.devType = ZDTYPE_SENSOR_BAROMETER;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Dew_Point)
		{
			_device.devType = ZDTYPE_SENSOR_DEWPOINT;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Power)
		{
			_device.devType = ZDTYPE_SENSOR_POWER;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Voltage)
		{
			_device.devType = ZDTYPE_SENSOR_VOLTAGE;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Current)
		{
			_device.devType = ZDTYPE_SENSOR_AMPERE;
		}
		else if (vLabel.find("Water") != std::string::npos) //water flow ? (ValueID_Index_SensorMultiLevel::Water_Cubic_Meters)
		{
			_device.devType = ZDTYPE_SENSOR_WATER;
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::Carbon_Dioxide)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Volatile_Organic_Compound)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Carbon_Monoxide)
			)
		{
			_device.devType = ZDTYPE_SENSOR_CO2;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Moisture)
		{
			_device.devType = ZDTYPE_SENSOR_MOISTURE;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Tank_Capacity)
		{
			_device.devType = ZDTYPE_SENSOR_TANK_CAPACITY;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Loudness)
		{
			_device.devType = ZDTYPE_SENSOR_LOUDNESS;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::General_Purpose)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				_device.custom_label = "?";
				_device.devType = ZDTYPE_SENSOR_CUSTOM;
			}
			else
				_device.devType = ZDTYPE_SWITCH_NORMAL;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Rain_Rate)
		{
			_device.custom_label = "mm/h";
			_device.devType = ZDTYPE_SENSOR_CUSTOM;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Particulate_Mater_2_5)
		{
			//dust
			_device.custom_label = "ug/m3";
			_device.devType = ZDTYPE_SENSOR_CUSTOM;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Seismic_Intensity)
		{
			_device.devType = ZDTYPE_SENSOR_PERCENTAGE;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Smoke_Density)
		{
			_device.devType = ZDTYPE_SENSOR_PERCENTAGE;
		}
		else if (
			(vOrgIndex == ValueID_Index_SensorMultiLevel::X_Axis_Acceleration)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Y_Axis_Acceleration)
			|| (vOrgIndex == ValueID_Index_SensorMultiLevel::Z_Axis_Acceleration)
			)
		{
			_device.instanceID = (uint8_t)vOrgIndex;
			_device.custom_label = "m/s2";
			_device.devType = ZDTYPE_SENSOR_CUSTOM;
		}
		else if (vOrgIndex == ValueID_Index_SensorMultiLevel::Frequency)
		{
			_device.custom_label = "Hz";
			_device.devType = ZDTYPE_SENSOR_CUSTOM;
		}
		else
		{
			_log.Log(LOG_STATUS, "OpenZWave: Value_Added: Unhandled Label: %s, Unit: %s, Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", vLabel.c_str(), vUnits.c_str(), static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), vLabel.c_str(), vOrgInstance, vOrgIndex);
			return;
		}
		_device.floatValue = fValue * _device.scaleMultiply;
		InsertDevice(_device);
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
				_device.devType = ZDTYPE_SENSOR_SETPOINT;
				InsertDevice(_device);
				SendDevice2Domoticz(&_device);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_MODE)
	{
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
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_STATE)
	{
		if (vOrgIndex == ValueID_Index_ThermostatFanState::FanState)
		{
			if (vType == OpenZWave::ValueID::ValueType_String)
			{
				std::string strValue;
				if (m_pManager->GetValueAsString(vID, &strValue) == true)
				{
					if (strValue == "Running")
					{
						_device.intvalue = 255;
					}
					else
					{   // default to Idle
						_device.intvalue = 0;
					}
					_device.devType = ZDTYPE_SWITCH_NORMAL;
					InsertDevice(_device);
					SendDevice2Domoticz(&_device);
				}
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_THERMOSTAT_OPERATING_STATE)
	{
		if (vOrgIndex == ValueID_Index_ThermostatOperatingState::OperatingState)
		{
			if (vType == OpenZWave::ValueID::ValueType_String)
			{
				std::string strValue;
				if (m_pManager->GetValueAsString(vID, &strValue) == true)
				{
					if (strValue == "Cooling")
					{
						_device.intvalue = 1;
					}
					else if (strValue == "Heating")
					{
						_device.intvalue = 2;
					}
					else
					{   // default to Idle
						_device.intvalue = 0;
					}
					_device.devType = ZDTYPE_SENSOR_THERMOSTAT_OPERATING_STATE;
					InsertDevice(_device);
					SendDevice2Domoticz(&_device);
				}
			}
		}
	}
	else if (commandclass == COMMAND_CLASS_CLOCK)
	{
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
				if ((pNode->tClockDay != UINT8_MAX) && (pNode->tClockHour != UINT8_MAX) && (pNode->tClockMinute != UINT8_MAX))
				{
					_log.Log(LOG_NORM, "OpenZWave: NodeID: %d (0x%02x), Thermostat Clock: %s %02d:%02d", NodeID, NodeID, ZWave_Clock_Days(pNode->tClockDay), pNode->tClockHour, pNode->tClockMinute);
					_device.intvalue = (pNode->tClockDay * (24 * 60)) + (pNode->tClockHour * 60) + pNode->tClockMinute;
					_device.devType = ZDTYPE_SENSOR_THERMOSTAT_CLOCK;
					InsertDevice(_device);
					SendDevice2Domoticz(&_device);
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE)
	{
		//Ignore this class, we can make our own schedule with timers
		return;
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
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
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
			(vOrgIndex == ValueID_Index_DoorLock::Lock)
			|| (vOrgIndex == ValueID_Index_DoorLock::Lock_Mode)
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
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				return; //Not in use at the moment
				int32 lValue = 0;
				if (m_pManager->GetValueListSelection(vID, &lValue) == false)
					return;
				if (
					(lValue == 0)
					|| (lValue == 1)
					)
				{
					//Locked/Locked Advanced
					_device.intvalue = 255;
				}
				else
				{
					//Unlock values?
					_device.intvalue = 0;
				}
				InsertDevice(_device);
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
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
	else if (commandclass == COMMAND_CLASS_INDICATOR)
	{
		//Ignored
		if (vType == OpenZWave::ValueID::ValueType_Byte)
		{
			if (m_pManager->GetValueAsByte(vID, &byteValue) == false)
				return;
		}
		else if (vType == OpenZWave::ValueID::ValueType_List)
		{
			std::vector<std::string > vStringList;
			if (m_pManager->GetValueListItems(vID, &vStringList) == false)
				return;
			if (m_pManager->GetValueListSelection(vID, &intValue) == false)
				return;
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
	}
	else if (commandclass == COMMAND_CLASS_SOUND_SWITCH)
	{
		if (vOrgIndex != ValueID_Index_SoundSwitch::Tones)
			return;
		if (vType != OpenZWave::ValueID::ValueType_List)
			return; //not interested in you

		std::vector<std::string > vStringList;
		if (m_pManager->GetValueListItems(vID, &vStringList) == false)
			return;
		_device.instanceID = instance;
		_device.devType = ZDTYPE_SWITCH_NORMAL;
		_device.intvalue = intValue;
		InsertDevice(_device);
	}
	else if (commandclass == COMMAND_CLASS_MANUFACTURER_PROPRIETARY)
	{
		//Could be anything
		if (
			(pNode->Manufacturer_id == 0x010F)
			&& (pNode->Product_id == 0x1000)
			&& (pNode->Product_type == 0x0302)
			)
		{
			//Fibaro FGRM222 Roller Shutter Controller 2
			//Slat/Tilt position
			if (vType == OpenZWave::ValueID::ValueType_Byte)
			{
				if (m_pManager->GetValueAsByte(vID, &byteValue) == true)
				{
					_device.instanceID = (uint8_t)vOrgIndex;
					_device.devType = ZDTYPE_SWITCH_DIMMER;
					_device.intvalue = byteValue;
					InsertDevice(_device);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
				return;
			}
		}
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
	if (m_pManager == nullptr)
		return;

	if (m_controllerID == 0)
		return;

	//if (m_nodesQueried==false)
	//return; //only allow updates when node Query is done

	//unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();
	uint8_t instance = vID.GetInstance();
	if (instance == 0)
		return;
	uint16_t index = vID.GetIndex();
	instance = (uint8_t)index;

	uint8_t commandclass = vID.GetCommandClassId();

	if (commandclass != COMMAND_CLASS_NO_OPERATION && !m_pManager->IsValueValid(vID))
		return;

	if ((commandclass == COMMAND_CLASS_ALARM) || (commandclass == COMMAND_CLASS_SENSOR_ALARM))
	{
		std::string vLabel;
		if (commandclass != 0)
		{
			vLabel = m_pManager->GetValueLabel(vID);
		}
		instance = GetIndexFromAlarm(vLabel);
		if (instance == 0)
			instance = (uint8_t)index;
	}

	_tZWaveDevice* pDevice = FindDevice(NodeID, instance, COMMAND_CLASS_SENSOR_BINARY, ZDTYPE_SWITCH_NORMAL);
	if (pDevice == nullptr)
	{
		//one more try
		pDevice = FindDevice(NodeID, instance, COMMAND_CLASS_SWITCH_BINARY, ZDTYPE_SWITCH_NORMAL);
		if (pDevice == nullptr)
		{
			// absolute last try
			instance = (uint8_t)vID.GetIndex();
			pDevice = FindDevice(NodeID, -1, COMMAND_CLASS_SENSOR_MULTILEVEL, ZDTYPE_SWITCH_NORMAL);
			if (pDevice == nullptr)
			{
				//okey, 1 more
				int tmp_instance = index;
				pDevice = FindDevice(NodeID, tmp_instance, COMMAND_CLASS_SWITCH_MULTILEVEL, ZDTYPE_SWITCH_DIMMER);
				if (pDevice == nullptr)
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
	time_t atime = mytime(nullptr);
	pDevice->intvalue = nintvalue;
	pDevice->lastreceived = atime;
	pDevice->sequence_number += 1;
	if (pDevice->sequence_number == 0)
		pDevice->sequence_number = 1;
	if (pDevice->bValidValue)
		SendDevice2Domoticz(pDevice);
}

void COpenZWave::UpdateValue(NodeInfo* pNode, const OpenZWave::ValueID& vID)
{
	if (m_pManager == nullptr)
		return;
	if (!m_pManager->IsValueValid(vID))
		return;

	if (m_controllerID == 0)
		return;

	//	if (m_nodesQueried==false)
	//	return; //only allow updates when node Query is done
	uint8_t commandclass = vID.GetCommandClassId();
	unsigned int HomeID = vID.GetHomeId();
	uint8_t NodeID = vID.GetNodeId();

	uint8_t instance = GetInstanceFromValueID(vID);
	//uint16_t index = vID.GetIndex();

	uint8_t vOrgInstance = vID.GetInstance();
	uint16_t vOrgIndex = vID.GetIndex();

	OpenZWave::ValueID::ValueType vType = vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre = vID.GetGenre();
	std::string vLabel = m_pManager->GetValueLabel(vID);
	std::string vUnits = m_pManager->GetValueUnits(vID);

	float fValue = 0;
	bool bValue = false;
	uint8_t byteValue = 0;
	int16 shortValue = 0;
	int32 intValue = 0;
	std::string strValue;
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
		_log.Log(LOG_ERROR, "OpenZWave: UpdateValue, Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
		return;
	}

	if (vGenre != OpenZWave::ValueID::ValueGenre_User)
	{
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

	time_t atime = mytime(nullptr);

	if ((commandclass == COMMAND_CLASS_ALARM) || (commandclass == COMMAND_CLASS_SENSOR_ALARM))
	{
		instance = GetIndexFromAlarm(vLabel);
		if (instance == 0)
			instance = (uint8_t)vOrgIndex;
	}

	std::stringstream sstr;
	sstr << int(NodeID) << ".instance." << int(vOrgInstance) << ".index." << vOrgIndex << ".commandClasses." << int(commandclass);
	std::string path = sstr.str();
	std::string path_plus = path + ".";

#ifdef DEBUG_ZWAVE_INT
	_log.Log(LOG_NORM, "OpenZWave: Value_Changed: Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
#endif

	//ignore the following command classes as they are not used in Domoticz at the moment
	if (
		(commandclass == COMMAND_CLASS_CLIMATE_CONTROL_SCHEDULE)
		|| (commandclass == COMMAND_CLASS_CENTRAL_SCENE && vOrgIndex == ValueID_Index_CentralScene::SceneCount)
		|| (commandclass == COMMAND_CLASS_SCENE_ACTIVATION && vOrgIndex == ValueID_Index_SceneActivation::Duration)
		|| (commandclass == COMMAND_CLASS_COLOR_CONTROL && vOrgIndex == ValueID_Index_Color::Index)
		|| (commandclass == COMMAND_CLASS_COLOR_CONTROL && vOrgIndex == ValueID_Index_Color::Channels_Capabilities)
		|| (commandclass == COMMAND_CLASS_COLOR_CONTROL && vOrgIndex == ValueID_Index_Color::Duration)
		
		)
	{
		return;
	}

	if (commandclass == COMMAND_CLASS_INDICATOR)
	{
		if (vType == OpenZWave::ValueID::ValueType_List)
		{
			std::vector<std::string > vStringList;
			if (m_pManager->GetValueListItems(vID, &vStringList) == false)
				return;
			if (m_pManager->GetValueListSelection(vID, &intValue) == false)
				return;
		}
		return; //not used right now
	}

	if (commandclass == COMMAND_CLASS_USER_CODE)
	{
		if ((instance == 1) && (vGenre == OpenZWave::ValueID::ValueGenre_User))
		{
			if (vOrgIndex == ValueID_Index_UserCode::Enrollment_Code)
			{
				//Enrollment Code
				if (vType == OpenZWave::ValueID::ValueType_String)
				{
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

					for (auto vNodeValue : pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values)
					{
						if ((vNodeValue.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (vNodeValue.GetInstance() == 1) && (vNodeValue.GetType() == OpenZWave::ValueID::ValueType_String))
						{
							int vNodeIndex = vNodeValue.GetIndex();
							if (vNodeIndex >= 1)
							{
								std::string sValue;
								if (m_pManager->GetValueAsString(vNodeValue, &sValue))
								{
									//Find Empty slot
									bool bDoAdd = sValue.empty();
									if (!bDoAdd)
										bDoAdd = sValue[0] == 0x00;
									if (bDoAdd)
									{
										_log.Log(LOG_STATUS, "OpenZWave: Enrolling User Code/Tag to code index: %d", vNodeIndex);
										m_pManager->SetValue(vNodeValue, strValue);
										AddValue(pNode, vID);
										bIncludedCode = true;
										break;
									}
								}
							}
						}
					}
					if (!bIncludedCode)
					{
						_log.Log(LOG_ERROR, "OpenZWave: Received new User Code/Tag, but there is no available room for new codes!!");
					}
				}
				m_bInUserCodeEnrollmentMode = false;
				return;
			}
			else if (vOrgIndex == ValueID_Index_UserCode::Refresh)
			{
				return;
			}
			else if (vOrgIndex == ValueID_Index_UserCode::RemoveCode)
			{
				return;
			}
			else if (vOrgIndex == ValueID_Index_UserCode::Count)
			{
				return;
			}
			else if (vOrgIndex == ValueID_Index_UserCode::RawValue)
			{
				return;
			}
			else if (vOrgIndex == ValueID_Index_UserCode::RawValueIndex)
			{
				//Tag/Code offset
				return;
			}
			else
			{
				//Normal Code Code_1/Code_253
				if ((vOrgIndex >= ValueID_Index_UserCode::Code_1) && (vOrgIndex <= ValueID_Index_UserCode::Code_253))
				{
					pNode->HaveUserCodes = true;

					if (vType == OpenZWave::ValueID::ValueType_String)
					{
						if (m_pManager->GetValueAsString(vID, &strValue) == true)
						{
						}
					}
				}
				return;
			}
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
				if ((pNode->tClockDay != UINT8_MAX) && (pNode->tClockHour != UINT8_MAX) && (pNode->tClockMinute != UINT8_MAX))
				{
					_log.Log(LOG_NORM, "OpenZWave: NodeID: %d (0x%02x), Thermostat Clock: %s %02d:%02d", NodeID, NodeID, ZWave_Clock_Days(pNode->tClockDay), pNode->tClockHour, pNode->tClockMinute);
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
			return;
		}
	}

	_tZWaveDevice *pDevice = nullptr;
	for (auto &m : m_devices)
	{
		std::string dstring = m.second.string_id;
		if (dstring == path) {
			pDevice = &m.second;
			break;
		}
		else {
			size_t loc = dstring.find(path_plus);
			if (loc != std::string::npos)
			{
				pDevice = &m.second;
				break;
			}
		}
	}
	if (pDevice == nullptr)
	{
		//New device, let's add it
		AddValue(pNode, vID);
		for (auto &device : m_devices)
		{
			std::string dstring = device.second.string_id;
			if (dstring == path) {
				pDevice = &device.second;
				break;
			}
			else {
				size_t loc = dstring.find(path_plus);
				if (loc != std::string::npos)
				{
					pDevice = &device.second;
					break;
				}
			}
		}
		if (pDevice == nullptr)
		{
			_log.Log(LOG_ERROR, "OpenZWave: Value_Changed: Tried adding value, not succeeded!. Node: %d (0x%02x), CommandClass: %s, Label: %s, Instance: %d, Index: %d", NodeID, NodeID, cclassStr(commandclass), vLabel.c_str(), vID.GetInstance(), vID.GetIndex());
			return;
		}
	}

	pDevice->bValidValue = true;

	if (
		(pDevice->Manufacturer_id == 0) &&
		(pDevice->Product_id == 0) &&
		(pDevice->Product_type == 0)
		)
	{
		if (!pNode->Manufacturer_name.empty())
		{
			pDevice->Manufacturer_id = pNode->Manufacturer_id;
			pDevice->Product_id = pNode->Product_id;
			pDevice->Product_type = pNode->Product_type;
		}
	}

	switch (pDevice->devType)
	{
	case ZDTYPE_SWITCH_NORMAL:
	{
		if (commandclass == COMMAND_CLASS_SENSOR_ALARM)
		{
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
			int32 intListValue = 0;
			std::string szListValue("??");
			if (vType == OpenZWave::ValueID::ValueType_List)
			{
				if (m_pManager->GetValueListSelection(vID, &intListValue))
				{
					if (intListValue == 0)
						intValue = 0;
					else
						intValue = 255;
					m_pManager->GetValueListSelection(vID, &szListValue);
					_log.Log(LOG_STATUS, "OpenZWave: Alarm received (%s: %s), NodeID: %d (0x%02x)", vLabel.c_str(), szListValue.c_str(), NodeID, NodeID);
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

			if (vOrgIndex == ValueID_Index_Alarm::Type_v1)
			{
				if (intListValue != 0)
				{
					m_LastAlarmTypeReceived = intListValue;
				}
				else
					m_LastAlarmTypeReceived = -1;
			}
			else if (vOrgIndex == ValueID_Index_Alarm::Level_v1)
			{
				if (m_LastAlarmTypeReceived != -1)
				{
					//Until we figured out what types/levels we have, we create a switch for each of them
					char szDeviceName[50];
					sprintf(szDeviceName, "Alarm Type: 0x%02X", m_LastAlarmTypeReceived);
					std::string tmpstr = szDeviceName;
					SendSwitch(NodeID, (uint8_t)m_LastAlarmTypeReceived, pDevice->batValue, (intValue != 0) ? true : false, 0, tmpstr, m_Name);
					m_LastAlarmTypeReceived = -1;
				}
			}
			else
			{
				if ((vOrgIndex > ValueID_Index_Alarm::Type_Start) && (vOrgIndex < ValueID_Index_Alarm::Type_Event_Param_Previous_Event))
				{
					char szTmp[100];
					sprintf(szTmp, "Alarm Type: %s %d (0x%02X)", vLabel.c_str(), vOrgIndex, vOrgIndex);
					SendZWaveAlarmSensor(pDevice->nodeID, pDevice->instanceID, pDevice->batValue, (uint8_t)vOrgIndex, intListValue, szListValue, szTmp);
				}

				if (vOrgIndex == ValueID_Index_Alarm::Type_Access_Control)
				{
					//Ignore "Clears" for Tag Readers, maybe it should be ignored for the complete access_control class ?
					if (intListValue == 0)
					{
						if (pDevice->Manufacturer_id == 0x008a)
						{
							//Benext
							if (pDevice->Product_type == 0x0007)
							{
								if (
									(pDevice->Product_id == 0x0100)
									|| (pDevice->Product_id == 0x0101)
									|| (pDevice->Product_id == 0x0200)
									)
									return;
							}
						}
						else if (pDevice->Manufacturer_id == 0x0097)
						{
							//Schlage
							if (pDevice->Product_type == 0x6131)
							{
								if (
									(pDevice->Product_id == 0x4101)
									|| (pDevice->Product_id == 0x4501)
									)
									return;
							}
						}
					}
					switch (intListValue) {
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
				else if (vOrgIndex == ValueID_Index_Alarm::Type_Power_Management)
				{
					switch (intListValue) {
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;

					case 0x0a:	// Replace battery soon
					case 0x0b:	// Replace battery now
					case 0x0e:	// Charge battery soon
					case 0x0f:	// Charge battery now!
						pDevice->batValue = 0; // signal battery needs attention ?!?
						// fall through by intent
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vOrgIndex == ValueID_Index_Alarm::Type_System)
				{
					switch (intListValue) {
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
				else if (vOrgIndex == ValueID_Index_Alarm::Type_Clock)
				{
					switch (intListValue) {
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
				else if (vOrgIndex == ValueID_Index_Alarm::Type_Home_Security)
				{
					switch (intListValue) {
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
				else if (vOrgIndex == ValueID_Index_Alarm::Type_Event_Param_UserCode)
				{
					if (vType == OpenZWave::ValueID::ValueType_Byte)
					{
						//byteValue contains Tag/Code offset
					}
					return; //not needed/handled
				}
				else
				{
					switch (intListValue)
					{
					case 0x00: 	// Previous Events cleared
					case 0xfe:	// Unkown event; returned when retrieving the current state.
						intValue = 0;
						break;
					default:	// all others, interpret as alarm
						intValue = 255;
						break;
					}
				}
			}
			pDevice->intvalue = intValue;
		}
		else if (commandclass == COMMAND_CLASS_DOOR_LOCK)
		{
			if (
				(vOrgIndex == ValueID_Index_DoorLock::Lock)
				|| (vOrgIndex == ValueID_Index_DoorLock::Lock_Mode)
				)
			{
				if (vType == OpenZWave::ValueID::ValueType_Bool)
				{
					if (m_pManager->GetValueAsBool(vID, &bValue) == true)
					{
						if (bValue == true)
							pDevice->intvalue = 255;
						else
							pDevice->intvalue = 0;
					}
				}
				else if (vType == OpenZWave::ValueID::ValueType_List)
				{
					return; //Not in use at the moment
					if (m_pManager->GetValueListSelection(vID, &lValue) == false)
						return;
					if (
						(lValue == 0)
						|| (lValue == 1)
						)
					{
						//Locked/Locked Advanced
						pDevice->intvalue = 255;
					}
					else
					{
						//Unlock values?
						pDevice->intvalue = 0;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, NodeID, NodeID);
					return;
				}
			}
		}
		else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_STATE)
		{
			if (vType != OpenZWave::ValueID::ValueType_String)
				return;
			if (vOrgIndex == ValueID_Index_ThermostatFanMode::FanMode)
			{
				if (m_pManager->GetValueAsString(vID, &strValue) == true)
				{
					if (strValue == "Running")
					{
						pDevice->intvalue = 255;
					}
					else
					{   // default to Idle
						pDevice->intvalue = 0;
					}
				}
			}
		}
		else if (commandclass == COMMAND_CLASS_SOUND_SWITCH)
		{
			if (vType == OpenZWave::ValueID::ValueType_List)
			{
				int32 listValue = 0;
				if (m_pManager->GetValueListSelection(vID, &listValue))
				{
					if (pDevice->intvalue == listValue)
					{
						return; //dont send same value
					}
					if (listValue == 0)
						pDevice->intvalue = 0;
					else
						pDevice->intvalue = 255;
				}
			}
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
					return;
				}
			}
			else if (vType == OpenZWave::ValueID::ValueType_List)
			{
				if (commandclass == COMMAND_CLASS_COLOR_CONTROL)
				{
					//Color Index changed, not used
					return;
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
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue * pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_POWERENERGYMETER:
	case ZDTYPE_SENSOR_KVAH:
	case ZDTYPE_SENSOR_KVAR:
	case ZDTYPE_SENSOR_KVARH:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (fValue < -1000000)
		{
			//NeoCoolcam reports values of -21474762
			_log.Log(LOG_ERROR, "OpenZWave: Invalid counter value received!: (%f) Node: %d (0x%02x), CommandClass: %s, Instance: %d, Index: %d, Id: 0x%llX", fValue,
					static_cast<int>(NodeID), static_cast<int>(NodeID), cclassStr(commandclass), pDevice->orgInstanceID, pDevice->orgIndexID, vID.GetId());
			return; //counter should not be negative
		}
		pDevice->floatValue = fValue * pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_TEMPERATURE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
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
		pDevice->intvalue = round(fValue);
		break;
	case ZDTYPE_SENSOR_UV:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_VELOCITY:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_BAROMETER:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue * pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_DEWPOINT:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_LIGHT:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vUnits == "%")
		{
			//convert from % to Lux (where max is 1000 Lux)
			fValue = (1000.0f / 100.0f) * fValue;
		}
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_VOLTAGE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_AMPERE:
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
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
		pDevice->floatValue = fValue;
		break;
	case ZDTYPE_SENSOR_GAS:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		float oldvalue = pDevice->floatValue;
		if ((fValue - oldvalue > 10.0f) || (fValue < oldvalue))
			return;//sanity check, don't report it
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_CO2:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_WATER:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_CUSTOM:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_MOISTURE:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_TANK_CAPACITY:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_LOUDNESS:
	{
		if (vType != OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue = fValue;
	}
	break;
	case ZDTYPE_SENSOR_THERMOSTAT_CLOCK:
		if (vOrgIndex == ValueID_Index_Clock::Minute)
		{
			pDevice->intvalue = (pNode->tClockDay * (24 * 60)) + (pNode->tClockHour * 60) + pNode->tClockMinute;
		}
		break;
	case ZDTYPE_SENSOR_THERMOSTAT_MODE:
		if (vType != OpenZWave::ValueID::ValueType_List)
			return;
		if (vOrgIndex == ValueID_Index_ThermostatMode::Mode)
		{
			pNode->tModes.clear();
			m_pManager->GetValueListItems(vID, &pNode->tModes);
			if (!pNode->tModes.empty())
			{
				std::string vListStr;
				if (!m_pManager->GetValueListSelection(vID, &vListStr))
					return;
				lValue = Lookup_ZWave_Thermostat_Modes(pNode->tModes, vListStr);
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
			pNode->tFanModes.clear();
			m_pManager->GetValueListItems(vID, &pNode->tFanModes);
			try
			{
				std::string vListStr;
				if (!m_pManager->GetValueListSelection(vID, &vListStr))
					return;
				lValue = Lookup_ZWave_Thermostat_Fan_Modes(vListStr);
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
	case ZDTYPE_SENSOR_THERMOSTAT_OPERATING_STATE:
		if (vType != OpenZWave::ValueID::ValueType_String)
			return;
		if (vOrgIndex == ValueID_Index_ThermostatFanMode::FanMode)
		{
			if (m_pManager->GetValueAsString(vID, &strValue) == true)
			{
				if (strValue == "Cooling")
				{
					pDevice->intvalue = 1;
				}
				else if (strValue == "Heating")
				{
					pDevice->intvalue = 2;
				}
				else
				{   // default to Idle
					pDevice->intvalue = 0;
				}
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
	if (m_pManager == nullptr)
		return false;
	try
	{
		CancelControllerCommand();
		m_LastIncludedNode = 0;
		m_LastIncludedNodeType = "";
		m_bHaveLastIncludedNodeInfo = false;
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = false; //unlimited time
		m_pManager->AddNode(m_controllerID, bSecure);
		_log.Log(LOG_STATUS, "OpenZWave: Node Include command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::ExcludeDevice(const uint8_t /*nodeID*/)
{
	try
	{
		if (m_pManager == nullptr)
			return false;
		CancelControllerCommand();
		m_LastRemovedNode = 0;
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = false; //unlimited time
		m_pManager->RemoveNode(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Node Exclude command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::IsHasNodeFailedDone()
{
	return (m_bHasNodeFailedDone);
}

bool COpenZWave::IsNodeReplaced()
{
	return (m_bNodeReplaced || (m_LastIncludedNode!=0 && m_bHaveLastIncludedNodeInfo));   // Succesful Node replace ends in Node_Added notification or Node_Alive notification
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
	if (m_pManager == nullptr)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::HardResetDevice()
{
	if (m_pManager == nullptr)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::HealNetwork()
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_pManager->HealNetwork(m_controllerID, true);
		_log.Log(LOG_STATUS, "OpenZWave: Heal Network command initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::HealNode(const uint8_t nodeID)
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_pManager->HealNetworkNode(m_controllerID, nodeID, true);
		_log.Log(LOG_STATUS, "OpenZWave: Heal Node command initiated for node: %d (0x%02x)...", nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::NetworkInfo(const int hwID, std::vector< std::vector< int > >& NodeArray)
{
	if (m_pManager == nullptr)
	{
		return false;
	}

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT HomeID,NodeID FROM ZWaveNodes WHERE (HardwareID = %d)",
		hwID);
	if (result.empty()) {
		return false;
	}
	int rowCnt = 0;
	for (const auto &sd : result)
	{
		unsigned int _homeID = static_cast<unsigned int>(std::stoul(sd[0]));

		if (_homeID != m_controllerID)
			continue;

		uint8_t _nodeID = (uint8_t)atoi(sd[1].c_str());
		if (COpenZWave::NodeInfo * nodeInfo = GetNodeInfo(_homeID, _nodeID))
		{
			std::vector<int> row;
			NodeArray.push_back(row);
			NodeArray[rowCnt].push_back(_nodeID);
			try
			{
				uint8* arr;
				int retval = m_pManager->GetNodeNeighbors(_homeID, _nodeID, &arr);
				if (retval > 0) {

					for (int i = 0; i < retval; i++) {
						NodeArray[rowCnt].push_back(arr[i]);
					}

					delete[] arr;
				}
			}
			catch (OpenZWave::OZWException& ex)
			{
				_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
					ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
					std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
				return false;
			}
			rowCnt++;
		}
	}
	return true;
}

int COpenZWave::ListGroupsForNode(const uint8_t nodeID)
{
	try
	{
		if (m_pManager == nullptr)
			return 0;
		return m_pManager->GetNumGroups(m_controllerID, nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return 0;
}

std::string COpenZWave::GetGroupName(const uint8_t nodeID, const uint8_t groupID)
{
	if (m_pManager == nullptr)
		return "";

	try
	{
		return m_pManager->GetGroupLabel(m_controllerID, nodeID, groupID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d", 
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return "";
}

int COpenZWave::ListAssociatedNodesinGroup(const uint8_t nodeID, const uint8_t groupID, std::vector<std::string>& nodesingroup)
{

	if (m_pManager == nullptr)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return 0;
}

bool COpenZWave::AddNodeToGroup(const uint8_t nodeID, const uint8_t groupID, const uint8_t addID, const uint8_t instance)
{

	if (m_pManager == nullptr)
		return false;
	try
	{
		m_pManager->AddAssociation(m_controllerID, nodeID, groupID, addID, instance);
		_log.Log(LOG_STATUS, "OpenZWave: added node: %d (0x%02x) instance %d in group: %d of node: %d (0x%02x)", addID, addID, instance, groupID, nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::RemoveNodeFromGroup(const uint8_t nodeID, const uint8_t groupID, const uint8_t removeID, const uint8_t instance)
{
	if (m_pManager == nullptr)
		return false;
	try
	{
		m_pManager->RemoveAssociation(m_controllerID, nodeID, groupID, removeID, instance);
		_log.Log(LOG_STATUS, "OpenZWave: removed node: %d (0x%02x) instance %d from group: %d of node: %d (0x%02x)", removeID, removeID, instance, groupID, nodeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::RemoveFailedDevice(const uint8_t nodeID)
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->RemoveFailedNode(m_controllerID, nodeID);
		_log.Log(LOG_STATUS, "OpenZWave: Remove Failed Device initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::ReceiveConfigurationFromOtherController()
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->ReceiveConfiguration(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Receive Configuration initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::SendConfigurationToSecondaryController()
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->ReplicationSend(m_controllerID, 0xFF);
		_log.Log(LOG_STATUS, "OpenZWave: Replication to Secondary Controller initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::TransferPrimaryRole()
{
	if (m_pManager == nullptr)
		return false;

	try
	{
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true;
		m_pManager->TransferPrimaryRole(m_controllerID);
		_log.Log(LOG_STATUS, "OpenZWave: Transfer Primary Role initiated...");
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
		m_bControllerCommandInProgress = false;
	}
	return false;
}

bool COpenZWave::CancelControllerCommand(const bool bForce)
{
	if ((m_bControllerCommandInProgress == false) && (!bForce))
		return false;
	if (m_pManager == nullptr)
		return false;

	m_bControllerCommandInProgress = false;
	m_bControllerCommandCanceled = true;
	try
	{
		return m_pManager->CancelControllerCommand(m_controllerID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

void COpenZWave::GetConfigFile(std::string& filePath, std::string& fileContent)
{
	std::string retstring;
	if (m_pManager == nullptr)
		return;

	std::lock_guard<std::mutex> l(m_NotificationMutex);

	char szFileName[255];
	sprintf(szFileName, "%sConfig/ozwcache_0x%08x.xml", szUserDataFolder.c_str(), m_controllerID);
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
	//OpenZWave::Driver::ControllerError err = (OpenZWave::Driver::ControllerError)_err;

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

void COpenZWave::EnableNodePoll(const unsigned int homeID, const uint8_t nodeID, const int /*pollTime*/)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == nullptr)
			return; //Not found

		bool bSingleIndexPoll = false;

		if (
			(pNode->Manufacturer_id == 0x0099)
			&& (pNode->Product_id == 0x0004)
			&& (pNode->Product_type == 0x0003)
			)
		{
			//GreenWave Reality Inc PowerNode 6
			bSingleIndexPoll = true;
		}

		for (const auto &ittInstance : pNode->Instances)
		{
			for (const auto &ittCmds : ittInstance.second)
			{
				for (const auto &ittValue : ittCmds.second.Values)
				{
					uint8_t commandclass = ittValue.GetCommandClassId();
					OpenZWave::ValueID::ValueGenre vGenre = ittValue.GetGenre();

					unsigned int _homeID = ittValue.GetHomeId();
					uint8_t _nodeID = ittValue.GetNodeId();
					uint16_t vOrgIndex = ittValue.GetIndex();
					if (m_pManager->IsNodeFailed(_homeID, _nodeID))
					{
						//do not enable/disable polling on nodes that are failed
						continue;
					}

					//Ignore non-user types
					if (vGenre != OpenZWave::ValueID::ValueGenre_User)
						continue;

					std::string vLabel = m_pManager->GetValueLabel(ittValue);

					if (
						(vLabel.find("Exporting") != std::string::npos) ||
						(vLabel.find("Interval") != std::string::npos) ||
						(vLabel.find("Previous Reading") != std::string::npos)
						)
						continue;

					if (commandclass == COMMAND_CLASS_SWITCH_BINARY)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_SWITCH_MULTILEVEL)
					{
						if (vOrgIndex == ValueID_Index_SwitchMultiLevel::Level)
						{
							if (!m_pManager->isPolled(ittValue))
								m_pManager->EnablePoll(ittValue, 1);
						}
					}
					else if (commandclass == COMMAND_CLASS_SENSOR_BINARY)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_METER)
					{
						//Meter device
						if (bSingleIndexPoll && (ittValue.GetIndex() != 0))
							continue;

						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_SENSOR_MULTILEVEL)
					{
						//if ((*ittValue).GetIndex() != 0)
						//{
						//	continue;
						//}
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 2);
					}
					else if (commandclass == COMMAND_CLASS_BATTERY)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 2);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_SETPOINT)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_MODE)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_FAN_STATE)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else if (commandclass == COMMAND_CLASS_THERMOSTAT_OPERATING_STATE)
					{
						if (!m_pManager->isPolled(ittValue))
							m_pManager->EnablePoll(ittValue, 1);
					}
					else
					{
						m_pManager->DisablePoll(ittValue);
					}
				}
			}
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::DisableNodePoll(const unsigned int homeID, const uint8_t nodeID)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == nullptr)
			return; //Not found

		for (const auto &ittInstance : pNode->Instances)
		{
			for (const auto &ittCmds : ittInstance.second)
			{
				for (const auto &ittValue : ittCmds.second.Values)
				{
					if (m_pManager->isPolled(ittValue))
						m_pManager->DisablePoll(ittValue);
				}
			}
		}
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::DeleteNode(const unsigned int homeID, const uint8_t nodeID)
{
	m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)", m_HwdID, homeID, nodeID);
}

void COpenZWave::AddNode(const unsigned int homeID, const uint8_t nodeID, const NodeInfo* pNode)
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
		if (pNode->Manufacturer_name.empty() || pNode->Product_name.empty())
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

void COpenZWave::SetNodeName(const unsigned int homeID, const uint8_t nodeID, const std::string& Name)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::EnableDisableNodePolling(const uint8_t nodeID)
{
	if (!m_pManager)
		return;

	try
	{
		int intervalseconds = 60;
		m_sql.GetPreferencesVar("ZWavePollInterval", intervalseconds);

		m_pManager->SetPollInterval(intervalseconds * 1000, false);

		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query("SELECT PollTime FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)", m_HwdID, m_controllerID, nodeID);
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::SetClock(const uint8_t nodeID, const uint8_t /*instanceID*/, const uint8_t /*commandClass*/, const uint8_t day, const uint8_t hour, const uint8_t minute)
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::SetThermostatMode(const uint8_t nodeID, const uint8_t instanceID, const uint8_t /*commandClass*/, const int tMode)
{
	if (m_pManager == nullptr)
		return;

	try
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			uint8_t vnodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, vnodeID);
			if (pNode)
			{
				if (tMode < (int)pNode->tModes.size())
				{
					m_pManager->SetValueListSelection(vID, pNode->tModes[tMode]);
				}
			}
		}
		m_updateTime = mytime(nullptr);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

void COpenZWave::SetThermostatFanMode(const uint8_t nodeID, const uint8_t instanceID, const uint8_t /*commandClass*/, const int fMode)
{
	if (m_pManager == nullptr)
		return;

	try
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_FAN_MODE, vID) == true)
		{
			m_pManager->SetValueListSelection(vID, ZWave_Thermostat_Fan_Modes[fMode]);
		}
		m_updateTime = mytime(nullptr);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
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
	uint8_t instanceID = ID4;
	int indexID = ID1;

	const _tZWaveDevice *pDevice = FindDevice((uint8_t)nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_MODE, ZDTYPE_SENSOR_THERMOSTAT_MODE);
	if (pDevice)
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass((uint8_t)nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			uint8_t vnodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, vnodeID);
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
	std::string retstr;
	uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
	uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
	uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
	uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

	int nodeID = (ID2 << 8) | ID3;
	uint8_t instanceID = ID4;
	int indexID = ID1;

	const _tZWaveDevice *pDevice = FindDevice((uint8_t)nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_FAN_MODE, ZDTYPE_SENSOR_THERMOSTAT_FAN_MODE);
	if (pDevice)
	{
		OpenZWave::ValueID vID(0, 0, OpenZWave::ValueID::ValueGenre_Basic, 0, 0, 0, OpenZWave::ValueID::ValueType_Bool);
		if (GetValueByCommandClass((uint8_t)nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_FAN_MODE, vID) == true)
		{
			unsigned int homeID = vID.GetHomeId();
			uint8_t vnodeID = vID.GetNodeId();
			COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, vnodeID);
			if (pNode)
			{
				int smode = 0;
				char szTmp[200];
				std::string modes;
				while (ZWave_Thermostat_Fan_Modes[smode] != nullptr)
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

void COpenZWave::NodeQueried(const unsigned int homeID, const uint8_t nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == nullptr)
		return;

	try
	{
		//All nodes have been queried, enable/disable node polling
		pNode->IsPlus = m_pManager->IsNodeZWavePlus(homeID, nodeID);
		EnableDisableNodePolling(nodeID);
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

bool COpenZWave::RequestNodeConfig(const unsigned int homeID, const uint8_t nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == nullptr)
		return false;

	try
	{
		m_pManager->RequestAllConfigParams(homeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::RequestNodeInfo(const unsigned int homeID, const uint8_t nodeID)
{
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == nullptr)
		return false;

	try
	{
		m_pManager->RefreshNodeInfo(homeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}


bool COpenZWave::HasNodeFailed(const unsigned int homeID, const uint8_t nodeID)
{
	// _log.Log(LOG_STATUS, "OpenZWave: Has node failed called for %u, %d",homeID, nodeID);
	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == nullptr)
		return false;

	try
	{
		m_bHasNodeFailedDone=false;
		m_sHasNodeFailedResult="";
		m_HasNodeFailedIdx=nodeID;

		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = true; 

		m_pManager->HasNodeFailed(homeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false; 
}


bool COpenZWave::ReplaceFailedNode(const unsigned int homeID, const uint8_t nodeID)
{
	// homeID=m_controllerID;
	// _log.Log(LOG_STATUS, "OpenZWave: Replace failed node called with home id (%u) and node id (%d)",homeID,nodeID);

	NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (pNode == nullptr)
	{
		// _log.Log(LOG_ERROR, "OpenZWave: Node not foud");
		return false;
	}
	try
	{
		CancelControllerCommand();

		m_bNodeReplaced=false;
		m_NodeToBeReplaced=nodeID;
		m_LastIncludedNode = 0;
		m_LastIncludedNodeType = "";
		m_bHaveLastIncludedNodeInfo = false;
		m_ControllerCommandStartTime = mytime(nullptr);
		m_bControllerCommandCanceled = false;
		m_bControllerCommandInProgress = false; //unlimited time
		m_pManager->ReplaceFailedNode(homeID, nodeID);
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false; 
}

void COpenZWave::GetNodeValuesJson(const unsigned int homeID, const uint8_t nodeID, Json::Value& root, const int index)
{
	if (!m_pManager)
		return;

	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == nullptr)
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
							_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, nodeID, nodeID);
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
							for (const auto &str : strs)
								root["result"][index]["config"][ivalue]["listitem"][vcounter++] = str;
						}
						uint16_t i_index = ittValue.GetIndex();
						if (commandclass == COMMAND_CLASS_PROTECTION)
						{
							i_index = 3000 + ittValue.GetInstance();
						}

						std::string i_label = m_pManager->GetValueLabel(ittValue);
						std::string i_units = m_pManager->GetValueUnits(ittValue);
						std::string i_help = m_pManager->GetValueHelp(ittValue);

						root["result"][index]["config"][ivalue]["index"] = i_index;
						root["result"][index]["config"][ivalue]["label"] = i_label;
						root["result"][index]["config"][ivalue]["units"] = i_units;
						root["result"][index]["config"][ivalue]["help"] = i_help;
						root["result"][index]["config"][ivalue]["LastUpdate"] = TimeToString(&ittCmds.second.m_LastSeen, TF_DateTime);
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

								root["result"][index]["config"][ivalue]["index"] = i_index;
								root["result"][index]["config"][ivalue]["label"] = i_label;
								root["result"][index]["config"][ivalue]["units"] = i_units;
								root["result"][index]["config"][ivalue]["help"] = i_help;
								root["result"][index]["config"][ivalue]["LastUpdate"] = TimeToString(&ittCmds.second.m_LastSeen, TF_DateTime);
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
}

bool COpenZWave::ApplyNodeConfig(const unsigned int homeID, const uint8_t nodeID, const std::string& svaluelist)
{
	try
	{
		NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (pNode == nullptr)
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
					std::string old_networkkey;
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
							m_sql.UpdatePreferencesVar("ZWaveNetworkKey", networkkey);
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
							_log.Log(LOG_ERROR, "OpenZWave: Unhandled value type: %d, %s:%d, NodeID: %d (0x%02x)", vType, std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__, nodeID, nodeID);
							return false;
						}
						if (!bRet)
						{
							std::string cvLabel = m_pManager->GetValueLabel(vID);
							_log.Log(LOG_ERROR, "OpenZWave: Error setting value: %s (%s), NodeID: %d (0x%02x)", cvLabel.c_str(), ValueVal.c_str(), nodeID, nodeID);
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
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

//User Code routines
bool COpenZWave::SetUserCodeEnrollmentMode()
{
	m_ControllerCommandStartTime = mytime(nullptr);
	m_bControllerCommandInProgress = true;
	m_bControllerCommandCanceled = false;
	m_bInUserCodeEnrollmentMode = true;
	_log.Log(LOG_STATUS, "OpenZWave: User Code Enrollment mode initiated...");
	return false;
}

bool COpenZWave::GetNodeUserCodes(const unsigned int homeID, const uint8_t nodeID, Json::Value& root)
{
	try
	{
		int ii = 0;
		COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
		if (!pNode)
			return false;
		if (pNode->Instances.find(1) == pNode->Instances.end())
			return false; //no codes added yet, wake your tag reader

		for (const auto &valueNode : pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values)
		{
			int vNodeIndex = valueNode.GetIndex();
			if ((valueNode.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (valueNode.GetInstance() == 1) && (valueNode.GetType() == OpenZWave::ValueID::ValueType_String))
			{
				if (vNodeIndex >= 1)
				{
					std::string sValue;
					if (m_pManager->GetValueAsString(valueNode, &sValue))
					{
						if (!sValue.empty())
						{
							if (sValue[0] != 0)
							{
								//Convert it to hex string, just in case
								std::string szHex = ToHexString((uint8_t*)sValue.c_str(), sValue.size());
								root["result"][ii]["index"] = vNodeIndex;
								root["result"][ii]["code"] = szHex;
								ii++;
							}
						}
					}
				}
			}
		}
		return true;
	}
	catch (OpenZWave::OZWException& ex)
	{
		_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
			ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
			std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
	}
	return false;
}

bool COpenZWave::RemoveUserCode(const unsigned int homeID, const uint8_t nodeID, const int codeIndex)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(homeID, nodeID);
	if (!pNode)
		return false;
	if (pNode->Instances.find(1) == pNode->Instances.end())
		return false; //no codes added yet, wake your tag reader

	for (const auto &value : pNode->Instances[1][COMMAND_CLASS_USER_CODE].Values)
	{
		if (value.GetIndex() == ValueID_Index_UserCode::RemoveCode)
			m_pManager->SetValue(value, (uint16_t)codeIndex);
		/*
				if ((vNodeValue.GetGenre() == OpenZWave::ValueID::ValueGenre_User) && (vNodeValue.GetInstance() == 1) && (vNodeValue.GetType() == OpenZWave::ValueID::ValueType_String))
				{
					int vNodeIndex = vNodeValue.GetIndex();
					if (vNodeIndex == codeIndex)
					{
						try
						{
							std::string sValue;
							sValue.assign(4, 0);
							m_pManager->SetValue(vNodeValue, sValue);
							break;
						}
						catch (OpenZWave::OZWException& ex)
						{
							_log.Log(LOG_ERROR, "OpenZWave: Exception. Type: %d, Msg: %s, File: %s (Line %d) %s:%d",
								ex.GetType(), ex.GetMsg().c_str(), ex.GetFile().c_str(), ex.GetLine(),
								std::string(__MYFUNCTION__).substr(std::string(__MYFUNCTION__).find_last_of("/\\") + 1).c_str(), __LINE__);
							return false;
						}
					}
				}
		*/
	}
	return true;
}

void COpenZWave::UpdateDeviceBatteryStatus(const uint8_t nodeID, const int value)
{
	COpenZWave::NodeInfo* pNode = GetNodeInfo(m_controllerID, nodeID);
	if (pNode)
		pNode->batValue = value;

	for (auto &device : m_devices)
		if (device.second.nodeID == nodeID)
			device.second.batValue = value;
	/*
		time_t now = time(0);
		struct tm ltime;
		localtime_r(&now, &ltime);
	*/
	//Update all devices in the database
	std::vector<std::vector<std::string> > results;
	results = m_sql.safe_query("SELECT ID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d)", m_HwdID);
	for (const auto &r : results)
	{
		std::string DeviceID = r[1];
		std::string rdID;

		if (DeviceID.size() != 8)
		{
			if (DeviceID.size() > 4)
				rdID = DeviceID.substr(DeviceID.size() - 4, 2);
			else
			{
				std::stringstream stream;
				stream << std::setfill('0') << std::setw(4) << std::hex << atoi(DeviceID.c_str());
				rdID = stream.str().substr(0, 2);
			}
		}
		else
		{
			rdID = DeviceID.substr(2, 2);
			if (rdID == "00")
				rdID = DeviceID.substr(4, 2);
		}

		int dev_nodeID;
		std::stringstream ss;
		ss << std::hex << rdID;
		ss >> dev_nodeID;

		if (dev_nodeID == nodeID)
		{
			/*
						m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, LastUpdate='%04d-%02d-%02d
			   %02d:%02d:%02d' WHERE (ID==%s)", value, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour,
			   ltime.tm_min, ltime.tm_sec, r[0].c_str());
			*/
			m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d WHERE (ID==%s)", value, r[0].c_str());
		}
	}
}


bool COpenZWave::GetBatteryLevels(Json::Value& root)
{
	int ii = 0;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT NodeID,Name FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u)", m_HwdID, m_controllerID);

	std::map<uint16_t, std::string> _NodeNames;
	for (const auto &r : result)
	{
		uint16_t nodeID = (uint16_t)atoi(r[0].c_str());
		_NodeNames[nodeID] = r[1];
	}

	for (const auto &node : m_nodes)
	{
		root["result"][ii]["nodeID"] = node.nodeId;
		std::string nodeName;
		if (_NodeNames.find(node.nodeId) != _NodeNames.end())
			nodeName = _NodeNames[node.nodeId];
		root["result"][ii]["nodeName"] = nodeName;
		root["result"][ii]["battery"] = node.batValue;
		ii++;
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

bool COpenZWave::IsNodeRGBW(const unsigned int homeID, const int nodeID)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ProductDescription FROM ZWaveNodes WHERE (HardwareID==%d) AND (HomeID==%u) AND (NodeID==%d)",
		m_HwdID, homeID, nodeID);
	if (result.empty())
		return false;
	std::string ProductDescription = result[0][0];
	return (
		(ProductDescription.find("FGRGBWM441") != std::string::npos)
		);
}

void COpenZWave::ForceUpdateForNodeDevices(const unsigned int homeID, const int nodeID)
{
	for (auto &device : m_devices)
	{
		if (device.second.nodeID == nodeID)
		{
			device.second.lastreceived = mytime(nullptr) - 1;

			_tZWaveDevice zdevice = device.second;

			SendDevice2Domoticz(&zdevice);

			if (zdevice.commandClassID == COMMAND_CLASS_SWITCH_MULTILEVEL)
			{
				if (zdevice.instanceID == 1)
				{
					if (IsNodeRGBW(homeID, nodeID))
					{
						zdevice.devType = ZDTYPE_SWITCH_RGBW;
						zdevice.instanceID = 100;
						SendDevice2Domoticz(&zdevice);
					}
				}
			}
			else if (zdevice.commandClassID == COMMAND_CLASS_COLOR_CONTROL)
			{
				zdevice.devType = ZDTYPE_SWITCH_COLOR;
				zdevice.instanceID = 101;
				SendDevice2Domoticz(&zdevice);
			}
		}
	}
}


//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::RType_OpenZWaveNodes(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == nullptr)
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
				int ii = 0;
				for (const auto &sd : result)
				{
					unsigned int homeID = static_cast<unsigned int>(std::stoul(sd[1]));
					if (homeID != pOZWHardware->GetControllerID())
						continue;
					uint8 nodeID = (uint8)atoi(sd[2].c_str());
					//if (nodeID>1) //Don't include the controller
					{
						COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == nullptr)
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
						root["result"][ii]["Battery"] = (pNode->batValue == 255) ? "-" : std::to_string(pNode->batValue);

						root["result"][ii]["LastUpdate"] = TimeToString(&pNode->LastSeen, TF_DateTime);

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
			if (idx.empty())
				return;
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string senablepolling = request::findValue(&req, "EnablePolling");
			if (name.empty() || senablepolling.empty())
				return;

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT HardwareID, HomeID, NodeID from ZWaveNodes WHERE (ID==%s)", idx.c_str());
			if (result.empty())
				return; //not found!?

			root["status"] = "OK";
			root["title"] = "UpdateZWaveNode";

			int hwid = atoi(result[0][0].c_str());
			unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
			uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(hwid);
			if (pHardware == nullptr)
				return; //not found!?
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;

			m_sql.safe_query("UPDATE ZWaveNodes SET Name='%q', PollTime=%d WHERE (ID=='%q')", name.c_str(), (senablepolling == "true") ? 1 : 0, idx.c_str());
			pOZWHardware->SetNodeName(homeID, nodeID, name);
			pOZWHardware->EnableDisableNodePolling(nodeID);
		}

		void CWebServer::Cmd_ZWaveDeleteNode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (result.empty())
				return;
			int hwid = atoi(result[0][0].c_str());
			//unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
			uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "DeleteZWaveNode";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->RemoveFailedDevice(nodeID);
			result = m_sql.safe_query("DELETE FROM ZWaveNodes WHERE (ID=='%q')", idx.c_str());
		}

		void CWebServer::Cmd_ZWaveInclude(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string ssecure = request::findValue(&req, "secure");
			bool bSecure = (ssecure == "true");
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveInclude";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			m_sql.AllowNewHardwareTimer(5);
			pOZWHardware->IncludeDevice(bSecure);
		}

		void CWebServer::Cmd_ZWaveExclude(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveExclude";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->ExcludeDevice(1);
		}

		void CWebServer::Cmd_ZWaveIsHasNodeFailedDone(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveIsHasNodeFailedDone";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			bool bIsHasNodeFailedDone = pOZWHardware->IsHasNodeFailedDone();
			root["result"] = bIsHasNodeFailedDone;
			if (bIsHasNodeFailedDone)
			{
				root["node_id"] = pOZWHardware->m_HasNodeFailedIdx;
				root["status_text"] = pOZWHardware->m_sHasNodeFailedResult;
			}
		}

		void CWebServer::Cmd_ZWaveIsNodeReplaced(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveIsNodeReplaced";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			bool bIsReplaced = pOZWHardware->IsNodeReplaced();
			root["result"] = bIsReplaced;
			if (bIsReplaced)
			{
				root["node_id"] = pOZWHardware->m_NodeToBeReplaced;
			}
		}

		void CWebServer::Cmd_ZWaveIsNodeIncluded(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveIsNodeIncluded";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			bool bIsIncluded = pOZWHardware->IsNodeIncluded();
			root["result"] = bIsIncluded;
			if (bIsIncluded)
			{
				root["node_id"] = pOZWHardware->m_LastIncludedNode;
				root["node_type"] = pOZWHardware->m_LastIncludedNodeType;
				std::string productName("Unknown");
				COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(pOZWHardware->GetControllerID(), (uint8_t)pOZWHardware->m_LastIncludedNode);
				if (pNode)
				{
					productName = pNode->Product_name;
				}
				root["node_product_name"] = productName;
			}
		}

		void CWebServer::Cmd_ZWaveIsNodeExcluded(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveIsNodeExcluded";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			root["result"] = pOZWHardware->IsNodeExcluded();
			root["node_id"] = (pOZWHardware->m_LastRemovedNode >0) ? std::to_string(pOZWHardware->m_LastRemovedNode) : "Failed!";
		}

		void CWebServer::Cmd_ZWaveSoftReset(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveSoftReset";
			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->SoftResetDevice();
		}

		void CWebServer::Cmd_ZWaveHardReset(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveHardReset";
			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->HardResetDevice();
		}

		void CWebServer::Cmd_ZWaveStateCheck(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["title"] = "ZWaveStateCheck";
			root["status"] = "OK";
			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			if (!pOZWHardware->GetFailedState()) {
			}
		}

		void CWebServer::Cmd_ZWaveNetworkHeal(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveHealNetwork";

			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->HealNetwork();
		}

		void CWebServer::Cmd_ZWaveNodeHeal(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::string node = request::findValue(&req, "node");
			if (node.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
			root["status"] = "OK";
			root["title"] = "ZWaveHealNode";
			COpenZWave *pOZWHardware = (COpenZWave *)pHardware;
			pOZWHardware->HealNode((uint8_t)atoi(node.c_str()));
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
			if (idx.empty())
				return;
			int hwID = atoi(idx.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwID);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::vector< std::vector< int > > nodevectors;

				if (pOZWHardware->NetworkInfo(hwID, nodevectors))
				{
					std::vector<int> allnodes;
					int rowCount = 0;
					std::stringstream allnodeslist;
					for (const auto &row : nodevectors)
					{
						int colCount = 0;
						int nodeID = -1;
						std::vector<int> rest;

						for (const auto &column : row)
						{
							if (colCount == 0) {
								nodeID = column;
							}
							else {
								rest.push_back(column);
							}
							colCount++;
						}

						if (nodeID != -1)
						{
							root["result"]["mesh"][rowCount]["nodeID"] = nodeID;

							std::stringstream list;
							for (const auto &r : rest)
							{
								if (!list.str().empty()) list << ",";
								list << std::to_string(r);
							}
							root["result"]["mesh"][rowCount]["seesNodes"] = list.str();
							rowCount++;
						}
					}
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
			if (idx.empty())
				return;
			std::string node = request::findValue(&req, "node");
			if (node.empty())
				return;
			std::string group = request::findValue(&req, "group");
			if (group.empty())
				return;
			std::string removenode = request::findValue(&req, "removenode");
			if (removenode.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				int nodeId = 0, instance = 0;
				sscanf(removenode.c_str(), "%d.%d", &nodeId, &instance);
				pOZWHardware->RemoveNodeFromGroup((uint8_t)atoi(node.c_str()), (uint8_t)atoi(group.c_str()), (uint8_t)nodeId, (uint8_t)instance);
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
			if (idx.empty())
				return;
			std::string node = request::findValue(&req, "node");
			if (node.empty())
				return;
			std::string group = request::findValue(&req, "group");
			if (group.empty())
				return;
			std::string addnode = request::findValue(&req, "addnode");
			if (addnode.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				int nodeId = 0, instance = 0;
				sscanf(addnode.c_str(), "%d.%d", &nodeId, &instance);
				pOZWHardware->AddNodeToGroup((uint8_t)atoi(node.c_str()), (uint8_t)atoi(group.c_str()), (uint8_t)nodeId, (uint8_t)instance);
				root["status"] = "OK";
				root["title"] = "ZWaveAddGroupNode";
			}
		}

		void CWebServer::Cmd_ZWaveGroupInfo(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			int iHardwareID = atoi(idx.c_str());

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT ID,HomeID,NodeID,Name FROM ZWaveNodes WHERE (HardwareID==%d)",
					iHardwareID);

				if (!result.empty())
				{
					int MaxNoOfGroups = 0;
					int ii = 0;

					for (const auto &sd : result)
					{
						unsigned int homeID = static_cast<unsigned int>(std::stoul(sd[1]));
						uint8_t nodeID = (uint8_t)atoi(sd[2].c_str());
						COpenZWave::NodeInfo* pNode = pOZWHardware->GetNodeInfo(homeID, nodeID);
						if (pNode == nullptr)
							continue;
						std::string nodeName = sd[3];
						int numGroups = pOZWHardware->ListGroupsForNode(nodeID);
						root["result"]["nodes"][ii]["nodeID"] = nodeID;
						root["result"]["nodes"][ii]["nodeName"] = (nodeName != "Unknown") ? nodeName : (pNode->Manufacturer_name + std::string(" ") + pNode->Product_name);
						root["result"]["nodes"][ii]["groupCount"] = numGroups;
						if (numGroups > 0)
						{
							if (numGroups > MaxNoOfGroups)
								MaxNoOfGroups = numGroups;

							for (int iGroup = 0; iGroup < numGroups; iGroup++)
							{
								root["result"]["nodes"][ii]["groups"][iGroup]["id"] = iGroup + 1;
								root["result"]["nodes"][ii]["groups"][iGroup]["groupName"] = pOZWHardware->GetGroupName(nodeID, (uint8_t)iGroup + 1);

								std::vector< std::string > nodesingroup;
								int numNodesInGroup = pOZWHardware->ListAssociatedNodesinGroup(nodeID, (uint8_t)iGroup + 1, nodesingroup);
								if (numNodesInGroup > 0) {
									std::stringstream list;
									std::copy(nodesingroup.begin(), nodesingroup.end(), std::ostream_iterator<std::string>(list, ","));
									root["result"]["nodes"][ii]["groups"][iGroup]["nodes"] = list.str();
								}
								else {
									root["result"]["nodes"][ii]["groups"][iGroup]["nodes"] = "";
								}
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

		void CWebServer::Cmd_ZWaveCancel(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
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
			if (idx.empty() || svaluelist.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->ApplyNodeConfig(homeID, nodeID, svaluelist))
						return;
					root["status"] = "OK";
					root["title"] = "ApplyZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveRequestNodeConfig(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeConfig(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeConfig";
				}
			}
		}

		void CWebServer::Cmd_ZWaveRequestNodeInfo(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->RequestNodeInfo(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "RequestZWaveNodeInfo";
				}
			}
		}

		void CWebServer::Cmd_ZWaveHasNodeFailed(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->HasNodeFailed(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "ZWaveHasNodeFailed";
				}
			}
		}

		void CWebServer::Cmd_ZWaveReplaceFailedNode(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					pOZWHardware->ReplaceFailedNode(homeID, nodeID);
					root["status"] = "OK";
					root["title"] = "ZWaveReplaceFailedNode";
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
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
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
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
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
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->TransferPrimaryRole();
				root["status"] = "OK";
				root["title"] = "ZWaveTransferPrimaryRole";
			}
		}
		void CWebServer::ZWaveGetConfigFile(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::string configFilePath;
				pOZWHardware->GetConfigFile(configFilePath, rep.content);
				if (!configFilePath.empty() && !rep.content.empty()) {
					std::string filename;
					std::size_t last_slash_pos = configFilePath.find_last_of('/');
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
		void CWebServer::ZWaveCPIndex(WebEmSession& /*session*/, const request& /*req*/, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				pOZWHardware->m_ozwcp.SetAllNodesChanged();
				std::string wwwFile = szWWWFolder + "/ozwcp/cp.html";
				reply::set_content_from_file(&rep, wwwFile);
			}
		}
		void CWebServer::ZWaveCPPollXml(WebEmSession& /*session*/, const request& /*req*/, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);

				reply::set_content(&rep, pOZWHardware->m_ozwcp.SendPollResponse());
				reply::add_header_attachment(&rep, "poll.xml");
			}
		}
		void CWebServer::ZWaveCPNodeGetConf(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			if (req.content.find("node") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sNode = request::findValue(&values, "node");
			if (sNode.empty())
				return;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.SendNodeConfResponse(iNode));
			}
		}
		void CWebServer::ZWaveCPNodeGetValues(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			if (req.content.find("node") == std::string::npos)
				return;
			std::multimap<std::string, std::string> values;
			request::makeValuesFromPostContent(&req, values);
			std::string sNode = request::findValue(&values, "node");
			if (sNode.empty())
				return;
			int iNode = atoi(sNode.c_str());
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.SendNodeValuesResponse(iNode));
			}
		}
		void CWebServer::ZWaveCPNodeSetValue(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			std::vector<std::string> strarray;
			StringSplit(req.content, "=", strarray);
			if (strarray.size() != 2)
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.SetNodeValue(strarray[0], strarray[1]));
			}
		}
		void CWebServer::ZWaveCPNodeSetButton(WebEmSession& /*session*/, const request& req, reply& rep)
		{
			std::vector<std::string> strarray;
			StringSplit(req.content, "=", strarray);
			if (strarray.size() != 2)
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.SetNodeButton(strarray[0], strarray[1]));
			}
		}
		void CWebServer::ZWaveCPAdminCommand(WebEmSession& /*session*/, const request& req, reply& rep)
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
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.DoAdminCommand(sFun, atoi(sNode.c_str()), atoi(sButton.c_str())));
			}
		}
		void CWebServer::ZWaveCPNodeChange(WebEmSession& /*session*/, const request& req, reply& rep)
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
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.DoNodeChange(sFun, atoi(sNode.c_str()), sValue));
			}
		}
		void CWebServer::ZWaveCPSetGroup(WebEmSession& /*session*/, const request& req, reply& rep)
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
				if (pHardware != nullptr)
				{
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
					reply::set_content(&rep, pOZWHardware->m_ozwcp.UpdateGroup(sFun, atoi(sNode.c_str()), atoi(sGroup.c_str()), glist));
				}
			}
		}
		void CWebServer::ZWaveCPGetTopo(WebEmSession& /*session*/, const request& /*req*/, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.GetCPTopo());
				reply::add_header_attachment(&rep, "topo.xml");
			}
		}
		void CWebServer::ZWaveCPGetStats(WebEmSession& /*session*/, const request& /*req*/, reply& rep)
		{
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(m_ZW_Hwidx);
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				std::lock_guard<std::mutex> l(pOZWHardware->m_NotificationMutex);
				reply::set_content(&rep, pOZWHardware->m_ozwcp.GetCPStats());
				reply::add_header_attachment(&rep, "stats.xml");
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
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_OpenZWave)
				return;
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

		void CWebServer::Cmd_ZWaveSetUserCodeEnrollmentMode(WebEmSession& session, const request& req, Json::Value& root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
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
			if (idx.empty() || scodeindex.empty())
				return;
			int iCodeIndex = atoi(scodeindex.c_str());

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID=='%q')", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->RemoveUserCode(homeID, nodeID, iCodeIndex))
						return;
					root["status"] = "OK";
					root["title"] = "RemoveUserCode";
				}
			}
		}

		void CWebServer::Cmd_ZWaveGetNodeUserCodes(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT HardwareID,HomeID,NodeID from ZWaveNodes WHERE (ID==%q)", idx.c_str());
			if (!result.empty())
			{
				int hwid = atoi(result[0][0].c_str());
				unsigned int homeID = static_cast<unsigned int>(std::stoul(result[0][1]));
				uint8_t nodeID = (uint8_t)atoi(result[0][2].c_str());
				CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(hwid);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType != HTYPE_OpenZWave)
						return;
					COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
					if (!pOZWHardware->GetNodeUserCodes(homeID, nodeID, root))
						return;
					root["status"] = "OK";
					root["title"] = "GetUserCodes";
				}
			}
		}

		void CWebServer::Cmd_ZWaveGetBatteryLevels(WebEmSession& /*session*/, const request& req, Json::Value& root)
		{
			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;

			CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(atoi(idx.c_str()));
			if (pHardware != nullptr)
			{
				if (pHardware->HwdType != HTYPE_OpenZWave)
					return;
				COpenZWave* pOZWHardware = (COpenZWave*)pHardware;
				if (!pOZWHardware->GetBatteryLevels(root))
					return;
				root["status"] = "OK";
				root["title"] = "GetBatteryLevels";
			}
		}

	} // namespace server
} // namespace http

#endif //WITH_OPENZWAVE
