#include "stdafx.h"
#ifdef WITH_OPENZWAVE
#include "OpenZWave.h"

#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <vector>
#include <ctype.h>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
#include "../main/mainworker.h"
#include "hardwaretypes.h"

#include "../json/json.h"
#include "../main/localtime_r.h"

//OpenZWave includes
#include "openzwave/Options.h"
#include "openzwave/Manager.h"
#include "openzwave/Driver.h"
#include "openzwave/Node.h"
#include "openzwave/Group.h"
#include "openzwave/Notification.h"
#include "openzwave/ValueStore.h"
#include "openzwave/Value.h"
#include "openzwave/ValueBool.h"
#include "openzwave/Log.h"

#include "ZWaveCommands.h"

//Note!, Some devices uses the same instance for multiple values,
//to solve this we are going to use the Index value!, Except for COMMAND_CLASS_MULTI_INSTANCE


#pragma warning(disable: 4996)

extern std::string szStartupFolder;

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	#define DEBUG_ZWAVE_INT
#endif

const char *cclassStr (uint8 cc)
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
		return "ZIP ADV SERVER";
	case 0x34:
		return "ZIP ADV CLIENT";
	case 0x35:
		return "METER PULSE";
	case 0x38:
		return "THERMOSTAT HEATING";
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

uint8 cclassNum (char const *str)
{
	if (strcmp(str, "NO OPERATION") == 0)
		return 0x00;
	else if (strcmp(str, "BASIC") == 0)
		return 0x20;
	else if (strcmp(str, "CONTROLLER REPLICATION") == 0)
		return 0x21;
	else if (strcmp(str, "APPLICATION STATUS") == 0)
		return 0x22;
	else if (strcmp(str, "ZIP SERVICES") == 0)
		return 0x23;
	else if (strcmp(str, "ZIP SERVER") == 0)
		return 0x24;
	else if (strcmp(str, "SWITCH BINARY") == 0)
		return 0x25;
	else if (strcmp(str, "SWITCH MULTILEVEL") == 0)
		return 0x26;
	else if (strcmp(str, "SWITCH ALL") == 0)
		return 0x27;
	else if (strcmp(str, "SWITCH TOGGLE BINARY") == 0)
		return 0x28;
	else if (strcmp(str, "SWITCH TOGGLE MULTILEVEL") == 0)
		return 0x29;
	else if (strcmp(str, "CHIMNEY FAN") == 0)
		return 0x2A;
	else if (strcmp(str, "SCENE ACTIVATION") == 0)
		return 0x2B;
	else if (strcmp(str, "SCENE ACTUATOR CONF") == 0)
		return 0x2C;
	else if (strcmp(str, "SCENE CONTROLLER CONF") == 0)
		return 0x2D;
	else if (strcmp(str, "ZIP CLIENT") == 0)
		return 0x2E;
	else if (strcmp(str, "ZIP ADV SERVICES") == 0)
		return 0x2F;
	else if (strcmp(str, "SENSOR BINARY") == 0)
		return 0x30;
	else if (strcmp(str, "SENSOR MULTILEVEL") == 0)
		return 0x31;
	else if (strcmp(str, "METER") == 0)
		return 0x32;
	else if (strcmp(str, "ZIP ADV SERVER") == 0)
		return 0x33;
	else if (strcmp(str, "ZIP ADV CLIENT") == 0)
		return 0x34;
	else if (strcmp(str, "METER PULSE") == 0)
		return 0x35;
	else if (strcmp(str, "THERMOSTAT HEATING") == 0)
		return 0x38;
	else if (strcmp(str, "THERMOSTAT MODE") == 0)
		return 0x40;
	else if (strcmp(str, "THERMOSTAT OPERATING STATE") == 0)
		return 0x42;
	else if (strcmp(str, "THERMOSTAT SETPOINT") == 0)
		return 0x43;
	else if (strcmp(str, "THERMOSTAT FAN MODE") == 0)
		return 0x44;
	else if (strcmp(str, "THERMOSTAT FAN STATE") == 0)
		return 0x45;
	else if (strcmp(str, "CLIMATE CONTROL SCHEDULE") == 0)
		return 0x46;
	else if (strcmp(str, "THERMOSTAT SETBACK") == 0)
		return 0x47;
	else if (strcmp(str, "DOOR LOCK LOGGING") == 0)
		return 0x4C;
	else if (strcmp(str, "SCHEDULE ENTRY LOCK") == 0)
		return 0x4E;
	else if (strcmp(str, "BASIC WINDOW COVERING") == 0)
		return 0x50;
	else if (strcmp(str, "MTP WINDOW COVERING") == 0)
		return 0x51;
	else if (strcmp(str, "MULTI INSTANCE") == 0)
		return 0x60;
	else if (strcmp(str, "DOOR LOCK") == 0)
		return 0x62;
	else if (strcmp(str, "USER CODE") == 0)
		return 0x63;
	else if (strcmp(str, "CONFIGURATION") == 0)
		return 0x70;
	else if (strcmp(str, "ALARM") == 0)
		return 0x71;
	else if (strcmp(str, "MANUFACTURER SPECIFIC") == 0)
		return 0x72;
	else if (strcmp(str, "POWERLEVEL") == 0)
		return 0x73;
	else if (strcmp(str, "PROTECTION") == 0)
		return 0x75;
	else if (strcmp(str, "LOCK") == 0)
		return 0x76;
	else if (strcmp(str, "NODE NAMING") == 0)
		return 0x77;
	else if (strcmp(str, "FIRMWARE UPDATE MD") == 0)
		return 0x7A;
	else if (strcmp(str, "GROUPING NAME") == 0)
		return 0x7B;
	else if (strcmp(str, "REMOTE ASSOCIATION ACTIVATE") == 0)
		return 0x7C;
	else if (strcmp(str, "REMOTE ASSOCIATION") == 0)
		return 0x7D;
	else if (strcmp(str, "BATTERY") == 0)
		return 0x80;
	else if (strcmp(str, "CLOCK") == 0)
		return 0x81;
	else if (strcmp(str, "HAIL") == 0)
		return 0x82;
	else if (strcmp(str, "WAKE UP") == 0)
		return 0x84;
	else if (strcmp(str, "ASSOCIATION") == 0)
		return 0x85;
	else if (strcmp(str, "VERSION") == 0)
		return 0x86;
	else if (strcmp(str, "INDICATOR") == 0)
		return 0x87;
	else if (strcmp(str, "PROPRIETARY") == 0)
		return 0x88;
	else if (strcmp(str, "LANGUAGE") == 0)
		return 0x89;
	else if (strcmp(str, "TIME") == 0)
		return 0x8A;
	else if (strcmp(str, "TIME PARAMETERS") == 0)
		return 0x8B;
	else if (strcmp(str, "GEOGRAPHIC LOCATION") == 0)
		return 0x8C;
	else if (strcmp(str, "COMPOSITE") == 0)
		return 0x8D;
	else if (strcmp(str, "MULTI INSTANCE ASSOCIATION") == 0)
		return 0x8E;
	else if (strcmp(str, "MULTI CMD") == 0)
		return 0x8F;
	else if (strcmp(str, "ENERGY PRODUCTION") == 0)
		return 0x90;
	else if (strcmp(str, "MANUFACTURER PROPRIETARY") == 0)
		return 0x91;
	else if (strcmp(str, "SCREEN MD") == 0)
		return 0x92;
	else if (strcmp(str, "SCREEN ATTRIBUTES") == 0)
		return 0x93;
	else if (strcmp(str, "SIMPLE AV CONTROL") == 0)
		return 0x94;
	else if (strcmp(str, "AV CONTENT DIRECTORY MD") == 0)
		return 0x95;
	else if (strcmp(str, "AV RENDERER STATUS") == 0)
		return 0x96;
	else if (strcmp(str, "AV CONTENT SEARCH MD") == 0)
		return 0x97;
	else if (strcmp(str, "SECURITY") == 0)
		return 0x98;
	else if (strcmp(str, "AV TAGGING MD") == 0)
		return 0x99;
	else if (strcmp(str, "IP CONFIGURATION") == 0)
		return 0x9A;
	else if (strcmp(str, "ASSOCIATION COMMAND CONFIGURATION") == 0)
		return 0x9B;
	else if (strcmp(str, "SENSOR ALARM") == 0)
		return 0x9C;
	else if (strcmp(str, "SILENCE ALARM") == 0)
		return 0x9D;
	else if (strcmp(str, "SENSOR CONFIGURATION") == 0)
		return 0x9E;
	else if (strcmp(str, "MARK") == 0)
		return 0xEF;
	else if (strcmp(str, "NON INTEROPERABLE") == 0)
		return 0xF0;
	else
		return 0xFF;
}

COpenZWave::COpenZWave(const int ID, const std::string& devname)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_controllerID=0;
	m_initFailed = false;
	m_nodesQueried = false;
	m_pManager=NULL;
}


COpenZWave::~COpenZWave(void)
{
	CloseSerialConnector();
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this notification
//-----------------------------------------------------------------------------
COpenZWave::NodeInfo* COpenZWave::GetNodeInfo( OpenZWave::Notification const* _notification )
{
	unsigned long const homeId = _notification->GetHomeId();
	unsigned char const nodeId = _notification->GetNodeId();
	for( std::list<NodeInfo>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
	{
		if( ( it->m_homeId == homeId ) && ( it->m_nodeId == nodeId ) )
		{
			return &(*it);
		}
	}

	return NULL;
}

COpenZWave::NodeInfo* COpenZWave::GetNodeInfo( const int homeID, const int nodeID)
{
	for( std::list<NodeInfo>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
	{
		if( ( it->m_homeId == homeID ) && ( it->m_nodeId == nodeID ) )
		{
			return &(*it);
		}
	}

	return NULL;
}

void OnDeviceStatusUpdate(OpenZWave::Driver::ControllerState cs, OpenZWave::Driver::ControllerError err, void *_context)
{
	COpenZWave *pClass=(COpenZWave*)_context;
	pClass->OnZWaveDeviceStatusUpdate(cs,err);
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification( OpenZWave::Notification const* _notification,void* _context)
{
	COpenZWave *pClass=(COpenZWave*)_context;
	pClass->OnZWaveNotification(_notification);
}

void COpenZWave::OnZWaveNotification( OpenZWave::Notification const* _notification)
{
	// Must do this inside a critical section to avoid conflicts with the main thread
	boost::lock_guard<boost::mutex> l(m_NotificationMutex);
	OpenZWave::Manager* pManager=OpenZWave::Manager::Get();
	if (!pManager)
		return;

	m_updateTime=mytime(NULL);

	OpenZWave::ValueID vID=_notification->GetValueID();
	int commandClass=vID.GetCommandClassId();
	unsigned long _homeID = _notification->GetHomeId();
	unsigned char _nodeID = _notification->GetNodeId();

	unsigned char instance;

	if (
		(commandClass==COMMAND_CLASS_MULTI_INSTANCE)||
		(commandClass==COMMAND_CLASS_SENSOR_MULTILEVEL)
		)
	{
		instance=vID.GetIndex();//(See note on top of this file) GetInstance();
	}
	else
	{
		instance=vID.GetInstance();//(See note on top of this file) GetInstance();
	}

	time_t act_time=mytime(NULL);

	switch( _notification->GetType() )
	{
	case OpenZWave::Notification::Type_DriverReady:
		{
			m_controllerID = _notification->GetHomeId();
			_log.Log(LOG_NORM,"OpenZWave: Driver Ready");
		}
		break;
	case OpenZWave::Notification::Type_NodeNew:
		_log.Log(LOG_NORM,"OpenZWave: New Node added. HomeID: %d, NodeID: %d",_homeID,_nodeID);
		break;
	case OpenZWave::Notification::Type_ValueAdded:
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			// Add the new value to our list
			nodeInfo->Instances[instance][commandClass].Values.push_back( vID );
			nodeInfo->m_LastSeen=act_time;
			nodeInfo->Instances[instance][commandClass].m_LastSeen=act_time;
			AddValue(vID);
		}
		break;
	case OpenZWave::Notification::Type_ValueRemoved:
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			// Remove the value from out list
			for( std::list<OpenZWave::ValueID>::iterator it = nodeInfo->Instances[instance][commandClass].Values.begin(); it != nodeInfo->Instances[instance][commandClass].Values.end(); ++it )
			{
				if( (*it) == vID )
				{
					nodeInfo->Instances[instance][commandClass].Values.erase( it );
					nodeInfo->Instances[instance][commandClass].m_LastSeen=act_time;
					nodeInfo->m_LastSeen=act_time;
					break;
				}
			}
		}
		break;
	case OpenZWave::Notification::Type_ValueChanged:
		// One of the node values has changed
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			nodeInfo->m_LastSeen=act_time;
			UpdateValue(vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_ValueRefreshed:
		// One of the node values has changed
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			//UpdateValue(vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_Notification:
	
		switch (_notification->GetNotification()) 
		{
		case OpenZWave::Notification::Code_Awake:
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->IsAwake=true;
			}
			break;
		case OpenZWave::Notification::Code_Sleep:
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->IsAwake=false;
			}
			break;
		case OpenZWave::Notification::Code_Dead:
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->IsDead=true;
			}
			break;
		}
		break;
	case OpenZWave::Notification::Type_Group:
		// One of the node's association groups has changed
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			nodeInfo->m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_NodeAdded:
		{
			// Add the new node to our list
			NodeInfo nodeInfo;
			nodeInfo.m_homeId = _homeID;
			nodeInfo.m_nodeId = _nodeID;
			nodeInfo.m_polled = false;
			nodeInfo.szType = m_pManager->GetNodeType(_homeID,_nodeID);
			nodeInfo.iVersion = m_pManager->GetNodeVersion(_homeID,_nodeID);
			nodeInfo.Manufacturer_id = m_pManager->GetNodeManufacturerId(_homeID,_nodeID);
			nodeInfo.Manufacturer_name = m_pManager->GetNodeManufacturerName(_homeID,_nodeID);
			nodeInfo.Product_type = m_pManager->GetNodeProductType(_homeID,_nodeID);
			nodeInfo.Product_id = m_pManager->GetNodeProductId(_homeID,_nodeID);
			nodeInfo.Product_name = m_pManager->GetNodeProductName(_homeID,_nodeID);

			nodeInfo.IsAwake=m_pManager->IsNodeAwake(_homeID,_nodeID);
			nodeInfo.IsDead=false;//m_pManager->IsNodeFailed(_homeID,_nodeID);

			nodeInfo.m_LastSeen=act_time;
			m_nodes.push_back( nodeInfo );
			AddNode(_homeID, _nodeID, &nodeInfo);
		}
		break;
	case OpenZWave::Notification::Type_NodeRemoved:
		{
			_log.Log(LOG_NORM,"OpenZWave: Node Removed. HomeID: %d, NodeID: %d",_homeID,_nodeID);
			// Remove the node from our list
			for( std::list<NodeInfo>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
			{
				if( ( it->m_homeId == _homeID ) && ( it->m_nodeId == _nodeID ) )
				{
					m_nodes.erase( it );
					break;
				}
			}
		}
		break;
	case OpenZWave::Notification::Type_NodeEvent:
		// We have received an event from the node, caused by a
		// basic_set or hail message.
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			nodeInfo->m_LastSeen=act_time;
			UpdateValue(vID);
			nodeInfo->Instances[instance][commandClass].m_LastSeen=act_time;
			nodeInfo->m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_PollingDisabled:
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			nodeInfo->m_polled = false;
			nodeInfo->m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_PollingEnabled:
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			nodeInfo->m_polled = true;
			nodeInfo->m_LastSeen=act_time;
		}
		break;
	case OpenZWave::Notification::Type_DriverFailed:
		{
			m_initFailed = true;
			_log.Log(LOG_ERROR,"OpenZWave: Driver Failed!!");
		}
		break;
	case OpenZWave::Notification::Type_AwakeNodesQueried:
	case OpenZWave::Notification::Type_AllNodesQueried:
	case OpenZWave::Notification::Type_AllNodesQueriedSomeDead:
		{
			m_nodesQueried = true;
			_log.Log(LOG_NORM,"OpenZWave: All Nodes queried");
			NodesQueried();
			OpenZWave::Manager::Get()->WriteConfig( m_controllerID );
			//IncludeDevice();
		}
		break;
	case OpenZWave::Notification::Type_NodeNaming:
		if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
		{
			std::string product_name=m_pManager->GetNodeProductName(_homeID,_nodeID);
			if (nodeInfo->Product_name!=product_name)
			{
				nodeInfo->Manufacturer_id = m_pManager->GetNodeManufacturerId(_homeID,_nodeID);
				nodeInfo->Manufacturer_name = m_pManager->GetNodeManufacturerName(_homeID,_nodeID);
				nodeInfo->Product_type = m_pManager->GetNodeProductType(_homeID,_nodeID);
				nodeInfo->Product_id = m_pManager->GetNodeProductId(_homeID,_nodeID);
				nodeInfo->Product_name = product_name;
				AddNode(_homeID,_nodeID,nodeInfo);
			}
			nodeInfo->m_LastSeen=act_time;
		}
		break;
/*
	case OpenZWave::Notification::Type_DriverReset:
	case OpenZWave::Notification::Type_NodeProtocolInfo:
	case OpenZWave::Notification::Type_NodeQueriesComplete:
	default:
		{
		}
		break;
*/
	}
}

void COpenZWave::StopHardwareIntern()
{
	//CloseSerialConnector();
}

void COpenZWave::EnableDisableDebug()
{
	int debugenabled=0;
#ifdef _DEBUG
	debugenabled=1;
#else
	m_pMainWorker->m_sql.GetPreferencesVar("ZWaveEnableDebug", debugenabled);
#endif

	if (debugenabled)
	{
		OpenZWave::Options::Get()->AddOptionInt( "SaveLogLevel", OpenZWave::LogLevel_Detail );
		OpenZWave::Options::Get()->AddOptionInt( "QueueLogLevel", OpenZWave::LogLevel_Debug );
		OpenZWave::Options::Get()->AddOptionInt( "DumpTrigger", OpenZWave::LogLevel_Error );
	}
	else
	{
		OpenZWave::Options::Get()->AddOptionInt( "SaveLogLevel", OpenZWave::LogLevel_Error );
		OpenZWave::Options::Get()->AddOptionInt( "QueueLogLevel", OpenZWave::LogLevel_Error );
		OpenZWave::Options::Get()->AddOptionInt( "DumpTrigger", OpenZWave::LogLevel_Error );
	}
	//if (!(OpenZWave::Options::Get()->GetOptionAsInt("DumpTriggerLevel",&optInt))) {
	//	OpenZWave::Options::Get()->AddOptionInt( "DumpTriggerLevel", OpenZWave::LogLevel_Error );
	//}

}

bool COpenZWave::OpenSerialConnector()
{
	_log.Log(LOG_NORM, "OpenZWave: Starting...");

	m_updateTime=mytime(NULL);
	CloseSerialConnector();
	std::string ConfigPath=szStartupFolder + "Config/";
	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	_log.Log(LOG_NORM, "OpenZWave: using config in: %s",ConfigPath.c_str());
	OpenZWave::Options::Create( ConfigPath.c_str(), ConfigPath.c_str(), "--SaveConfiguration=true " );
	EnableDisableDebug();
	OpenZWave::Options::Get()->AddOptionInt( "PollInterval", 60000 ); //enable polling each 60 seconds
	OpenZWave::Options::Get()->AddOptionBool( "IntervalBetweenPolls", true );
	OpenZWave::Options::Get()->AddOptionBool("ValidateValueChanges", true);
	OpenZWave::Options::Get()->Lock();

	OpenZWave::Manager::Create();
	m_pManager=OpenZWave::Manager::Get();
	if (m_pManager==NULL)
		return false;

	// Add a callback handler to the manager.  The second argument is a context that
	// is passed to the OnNotification method.  If the OnNotification is a method of
	// a class, the context would usually be a pointer to that class object, to
	// avoid the need for the notification handler to be a static.
	OpenZWave::Manager::Get()->AddWatcher( OnNotification, this );

	// Add a Z-Wave Driver
	// Modify this line to set the correct serial port for your PC interface.
#ifdef WIN32
	if (m_szSerialPort.size()<4)
		return false;
	std::string szPort=m_szSerialPort.substr(3);
	int iPort=atoi(szPort.c_str());
	char szComm[15];
	if (iPort<10)
		sprintf(szComm,"COM%d",iPort);
	else
		sprintf(szComm,"\\\\.\\COM%d",iPort);
	OpenZWave::Manager::Get()->AddDriver(szComm);
#else
	OpenZWave::Manager::Get()->AddDriver( m_szSerialPort.c_str() );
#endif
	//Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );
	return true;
}

void COpenZWave::CloseSerialConnector()
{
	// program exit (clean up)
	OpenZWave::Manager* pManager=OpenZWave::Manager::Get();
	if (pManager)
	{
//		boost::lock_guard<boost::mutex> l(m_NotificationMutex);
		_log.Log(LOG_NORM,"OpenZWave: Closed");
		OpenZWave::Manager::Get()->RemoveDriver(m_szSerialPort.c_str());
		OpenZWave::Manager::Get()->RemoveWatcher( OnNotification, this );

		OpenZWave::Manager::Destroy();
		OpenZWave::Options::Destroy();
		sleep_seconds(1);
		m_pManager=NULL;
	}
}

bool COpenZWave::GetInitialDevices()
{
	m_controllerID=0;
	m_initFailed = false;
	m_nodesQueried = false;

	//Connect and request initial devices
	OpenSerialConnector();

	return true;
}

bool COpenZWave::GetUpdates()
{
	if (m_pManager==NULL)
		return false;

	return true;
}

bool COpenZWave::GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID &nValue)
{
	COpenZWave::NodeInfo *pNode=GetNodeInfo( m_controllerID, nodeID);
	if (!pNode)
		return false;

	for( std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[instanceID][commandClass].Values.begin(); itt != pNode->Instances[instanceID][commandClass].Values.end(); ++itt )
	{
		unsigned char cmdClass=itt->GetCommandClassId();
		if( cmdClass == commandClass )
		{
			nValue=*itt;
			return true;
		}
	}
	return false;
}

bool COpenZWave::GetValueByCommandClassLabel(const int nodeID, const int instanceID, const int commandClass, const std::string &vLabel, OpenZWave::ValueID &nValue)
{
	COpenZWave::NodeInfo *pNode=GetNodeInfo( m_controllerID, nodeID);
	if (!pNode)
		return false;

	for( std::list<OpenZWave::ValueID>::iterator itt = pNode->Instances[instanceID][commandClass].Values.begin(); itt != pNode->Instances[instanceID][commandClass].Values.end(); ++itt )
	{
		unsigned char cmdClass=itt->GetCommandClassId();
		if( cmdClass == commandClass )
		{
			std::string cvLabel=m_pManager->GetValueLabel(*itt);
			if (cvLabel==vLabel)
			{
				nValue=*itt;
				return true;
			}
		}
	}
	return false;
}

bool COpenZWave::GetNodeConfigValueByIndex(const NodeInfo *pNode, const int index, OpenZWave::ValueID &nValue)
{
	for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance=pNode->Instances.begin(); ittInstance!=pNode->Instances.end(); ++ittInstance)
	{
		for( std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds )
		{
			for( std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue!= ittCmds->second.Values.end(); ++ittValue)
			{
				int vindex=ittValue->GetIndex();
				unsigned char commandclass=ittValue->GetCommandClassId();
				if( 
					(commandclass == COMMAND_CLASS_CONFIGURATION)&&
					(vindex==index)
					)
				{
					nValue=*ittValue;
					return true;
				}
			}
		}
	}
	return false;
}

void COpenZWave::SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)
{
	if (m_pManager==NULL)
		return;
	boost::lock_guard<boost::mutex> l(m_NotificationMutex);

	OpenZWave::ValueID vID(0,0,OpenZWave::ValueID::ValueGenre_Basic,0,0,0,OpenZWave::ValueID::ValueType_Bool);

	unsigned char svalue=(unsigned char)value;

	bool bIsDimmer=(GetValueByCommandClassLabel(nodeID, instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL,"Level",vID)==true);
	if (bIsDimmer==false)
	{
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_SWITCH_BINARY,vID)==true)
		{
			_log.Log(LOG_NORM,"OpenZWave: Domoticz has send a Switch command!");
			if (svalue==0) {
				//Off
				m_pManager->SetValue(vID,false);
			}
			else {
				//On
				m_pManager->SetValue(vID,true);
			}
		}
	}
	else
	{
/*
		if (vType == OpenZWave::ValueID::ValueType_Decimal)
		{
			float pastLevel;
			if (m_pManager->GetValueAsFloat(vID,&pastLevel)==true)
			{
				if (svalue==pastLevel)
				{
					if (GetValueByCommandClassLabel(nodeID, instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL,"Level",vID)==true);
				}
			}
		}
*/
		if (svalue>99)
			svalue=99;
		_log.Log(LOG_NORM,"OpenZWave: Domoticz has send a Switch command!, Level: %d",svalue);
		if (!m_pManager->SetValue(vID,svalue))
		{
			_log.Log(LOG_ERROR,"OpenZWave: Error setting Switch Value!");
		}
	}
	m_updateTime=mytime(NULL);
}

void COpenZWave::SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value)
{
	if (m_pManager==NULL)
		return;
	boost::lock_guard<boost::mutex> l(m_NotificationMutex);
	OpenZWave::ValueID vID(0,0,OpenZWave::ValueID::ValueGenre_Basic,0,0,0,OpenZWave::ValueID::ValueType_Bool);
	if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_THERMOSTAT_SETPOINT,vID)==true)
	{
		m_pManager->SetValue(vID,value);
	}
	m_updateTime=mytime(NULL);
}

void COpenZWave::AddValue(const OpenZWave::ValueID vID)
{
	if (m_pManager==NULL)
		return;
	if (m_controllerID==0)
		return;
	unsigned char commandclass=vID.GetCommandClassId();

	//Ignore some command classes, values are already added before calling this function
	if (
		(commandclass==COMMAND_CLASS_BASIC)||
		(commandclass==COMMAND_CLASS_SWITCH_ALL)||
		(commandclass==COMMAND_CLASS_CONFIGURATION)||
		(commandclass==COMMAND_CLASS_VERSION)||
		(commandclass==COMMAND_CLASS_POWERLEVEL)
		)
		return;

	unsigned char NodeID = vID.GetNodeId();

	unsigned char instance;
	if (
		(commandclass==COMMAND_CLASS_MULTI_INSTANCE)||
		(commandclass==COMMAND_CLASS_SENSOR_MULTILEVEL)
		)
	{
		instance=vID.GetIndex();//(See note on top of this file) GetInstance();
	}
	else
	{
		instance=vID.GetInstance();//(See note on top of this file) GetInstance();
	}

	OpenZWave::ValueID::ValueType vType=vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre=vID.GetGenre();

	//Ignore non-user types
	if (vGenre!=OpenZWave::ValueID::ValueGenre_User)
		return;

	std::string vLabel=m_pManager->GetValueLabel(vID);

	if (
		(vLabel=="Exporting")||
		(vLabel=="Interval")||
		(vLabel=="Previous Reading")
		)
		return;

	std::string vUnits=m_pManager->GetValueUnits(vID);
	_log.Log(LOG_NORM, "Value_Added: Node: %d, CommandClass: %s, Label: %s",(int)NodeID, cclassStr(commandclass),vLabel.c_str());

	_tZWaveDevice _device;
	_device.nodeID=NodeID;
	_device.commandClassID=commandclass;
	_device.scaleID=-1;
	_device.instanceID=instance;
	_device.hasWakeup=m_pManager->IsNodeAwake(m_controllerID,NodeID);
	_device.isListening=m_pManager->IsNodeListeningDevice(m_controllerID,NodeID);

	if ((_device.instanceID==0)&&(NodeID==m_controllerID))
		return;// We skip instance 0 if there are more, since it should be mapped to other instances or their superposition



	float fValue=0;
	int iValue=0;
	bool bValue=false;
	unsigned char byteValue=0;

	// We choose SwitchMultilevel first, if not available, SwhichBinary is chosen
	if (commandclass==COMMAND_CLASS_SWITCH_BINARY)
	{
		if ((vLabel=="Switch")||(vLabel=="Sensor"))
		{
			if (m_pManager->GetValueAsBool(vID,&bValue)==true)
			{
				_device.devType= ZDTYPE_SWITCHNORMAL;
				if (bValue==true)
					_device.intvalue=255;
				else
					_device.intvalue=0;
				InsertDevice(_device);
			}
			else if (m_pManager->GetValueAsByte(vID,&byteValue)==true)
			{
				if (byteValue==0)
					_device.intvalue=0;
				else
					_device.intvalue=255;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_SWITCH_MULTILEVEL)
	{
		if (vLabel=="Level")
		{
			if (m_pManager->GetValueAsByte(vID,&byteValue)==true)
			{
				_device.devType= ZDTYPE_SWITCHDIMMER;
				_device.intvalue=byteValue;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_SENSOR_BINARY)
	{
		if ((vLabel=="Switch")||(vLabel=="Sensor"))
		{
			if (m_pManager->GetValueAsBool(vID,&bValue)==true)
			{
				_device.devType= ZDTYPE_SWITCHNORMAL;
				if (bValue==true)
					_device.intvalue=255;
				else
					_device.intvalue=0;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_BASIC_WINDOW_COVERING)
	{
		if (vLabel=="Open")
		{
			_device.devType= ZDTYPE_SWITCHNORMAL;
			_device.intvalue=255;
			InsertDevice(_device);
		}
		else if (vLabel=="Close")
		{
			_device.devType= ZDTYPE_SWITCHNORMAL;
			_device.intvalue=0;
			InsertDevice(_device);
		}
	}
	else if ((commandclass==COMMAND_CLASS_ALARM)||(commandclass==COMMAND_CLASS_SENSOR_ALARM))
	{
		if (
			(vLabel=="Alarm Level")||
			(vLabel=="Flood")||
			(vLabel=="Smoke")||
			(vLabel=="Heat")
			)
		{
			if (m_pManager->GetValueAsByte(vID,&byteValue)==true)
			{
				_device.devType= ZDTYPE_SWITCHNORMAL;
				if (byteValue==0)
					_device.intvalue=0;
				else
					_device.intvalue=255;
				InsertDevice(_device);
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_METER)
	{
		//Meter device
		if (
			(vLabel=="Energy")||
			(vLabel=="Power")
			)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID,&fValue)==true)
				{
					if (vLabel=="Energy")
						_device.scaleID=1;
					else
						_device.scaleID=2;
					_device.floatValue=fValue;
					_device.scaleMultiply=1;
					if (vUnits=="kWh")
					{
						_device.scaleMultiply=1000;
						_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
					}
					else
					{
						_device.devType = ZDTYPE_SENSOR_POWER;
					}
					InsertDevice(_device);
				}
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_SENSOR_MULTILEVEL)
	{
		if (vLabel=="Temperature")
		{
			if (m_pManager->GetValueAsFloat(vID,&fValue)==true)
			{
				if (vUnits=="F")
				{
					//Convert to celcius
					fValue=float((fValue-32)*(5.0/9.0));
				}
				_device.floatValue=fValue;
				_device.commandClassID=49;
				_device.devType = ZDTYPE_SENSOR_TEMPERATURE;
				InsertDevice(_device);
			}
		}
		else if (vLabel=="Luminance")
		{
			if (m_pManager->GetValueAsFloat(vID,&fValue)==true)
			{
				_device.floatValue=fValue;
				_device.commandClassID=49;
				_device.devType = ZDTYPE_SENSOR_LIGHT;
				InsertDevice(_device);
			}
		}
		else if (vLabel=="Relative Humidity")
		{
			if (m_pManager->GetValueAsFloat(vID,&fValue)==true)
			{
				_device.intvalue=int(fValue);
				_device.commandClassID=49;
				_device.devType = ZDTYPE_SENSOR_HUMIDITY;
				InsertDevice(_device);
			}
		}
		else if (
			(vLabel=="Energy")||
			(vLabel=="Power")
			)
		{
			if (vType == OpenZWave::ValueID::ValueType_Decimal)
			{
				if (m_pManager->GetValueAsFloat(vID,&fValue)==true)
				{
					if (vLabel=="Energy")
						_device.scaleID=1;
					else
						_device.scaleID=2;
					_device.floatValue=fValue;
					_device.scaleMultiply=1;
					if (vUnits=="kWh")
					{
						_device.scaleMultiply=1000;
						_device.devType = ZDTYPE_SENSOR_POWERENERGYMETER;
					}
					else
					{
						_device.devType = ZDTYPE_SENSOR_POWER;
					}
					InsertDevice(_device);
				}
			}
		}
	}
	else if (commandclass==COMMAND_CLASS_BATTERY)
	{
		if (_device.isListening)
		{
			if (vType== OpenZWave::ValueID::ValueType_Byte)
			{
				UpdateDeviceBatteryStatus(NodeID,byteValue);
			}
		}
	}
	else
	{
		//Unhanded
		_log.Log(LOG_NORM, "OpenZWave: Unhanded class: 0x%02X",commandclass);
		if (vType== OpenZWave::ValueID::ValueType_List)
		{
			//std::vector<std::string > vStringList;
			//if (m_pManager->GetValueListItems(vID,&vStringList)==true)
			//{
			//}
		}
	}
}

void COpenZWave::UpdateValue(const OpenZWave::ValueID vID)
{
	if (m_pManager==NULL)
		return;
	if (m_controllerID==0)
		return;

	if (m_nodesQueried==false)
		return; //only allow updates when node Query is done
	unsigned char commandclass=vID.GetCommandClassId();
	unsigned char HomeID = vID.GetHomeId();
	unsigned char NodeID = vID.GetNodeId();

	unsigned char instance;
	if (
		(commandclass==COMMAND_CLASS_MULTI_INSTANCE)||
		(commandclass==COMMAND_CLASS_SENSOR_MULTILEVEL)
		)
	{
		instance=vID.GetIndex();//(See note on top of this file) GetInstance();
	}
	else
	{
		instance=vID.GetInstance();//(See note on top of this file) GetInstance();
	}

	OpenZWave::ValueID::ValueType vType=vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre=vID.GetGenre();
	std::string vLabel=m_pManager->GetValueLabel(vID);
	std::string vUnits=m_pManager->GetValueUnits(vID);

	if (vGenre!=OpenZWave::ValueID::ValueGenre_User)
	{
		NodeInfo *pNode=GetNodeInfo(m_controllerID,NodeID);
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
		if ((pNode)&&(vLabel=="Wake-up Interval"))
		{
			if (HomeID!=m_controllerID)
				m_pManager->AddAssociation(m_controllerID,NodeID,1,1);
		}
		return;
	}

	if (
		(vLabel=="Exporting")||
		(vLabel=="Interval")||
		(vLabel=="Previous Reading")
		)
		return;

	time_t atime=mytime(NULL);
	std::stringstream sstr;
	sstr << int(NodeID) << ".instances." << int(instance) << ".commandClasses." << int(commandclass) << ".data";

	if (
		(vLabel=="Energy")||
		(vLabel=="Power")
		)
	{
		int scaleID=0;
		if (vLabel=="Energy")
			scaleID=1;
		else
			scaleID=2;

		sstr << "." << scaleID;
	}

	std::string path=sstr.str();
#ifdef _DEBUG
	_log.Log(LOG_NORM, "Value_Changed: Node: %d, CommandClass: %s, Label: %s, Instance: %d, Index: %d",NodeID, cclassStr(commandclass),vLabel.c_str(),vID.GetInstance(),vID.GetIndex());
#endif
	float fValue=0;
	int iValue=0;
	bool bValue=false;
	unsigned char byteValue=0;

	if (vType== OpenZWave::ValueID::ValueType_Decimal)
	{
		if (m_pManager->GetValueAsFloat(vID,&fValue)==false)
			return;
	}
	if (vType== OpenZWave::ValueID::ValueType_Bool)
	{
		if (m_pManager->GetValueAsBool(vID,&bValue)==false)
			return;
	}
	if (vType== OpenZWave::ValueID::ValueType_Byte)
	{
		if (m_pManager->GetValueAsByte(vID,&byteValue)==false)
			return;
	}

	if (commandclass==COMMAND_CLASS_BATTERY)
	{
		//Battery status update
		if (vType==OpenZWave::ValueID::ValueType_Byte)
		{
			UpdateDeviceBatteryStatus(NodeID,byteValue);
		}
		return;
	}

	_tZWaveDevice *pDevice=NULL;
	std::map<std::string,_tZWaveDevice>::iterator itt;
	for (itt=m_devices.begin(); itt!=m_devices.end(); ++itt)
	{
		std::string::size_type loc = path.find(itt->second.string_id,0);
		if (loc!=std::string::npos)
		{
			pDevice=&itt->second;
			break;
		}
	}
	if (pDevice==NULL)
	{
		return;
	}

	pDevice->bValidValue=true;

	switch (pDevice->devType)
	{
	case ZDTYPE_SWITCHNORMAL:
		{
			if ((vLabel=="Alarm Level")||(vLabel=="Flood")||(vLabel=="Smoke")||(vLabel=="Heat"))
			{
				int nintvalue=0;
				if (byteValue==0)
					nintvalue=0;
				else
					nintvalue=255;
				if (nintvalue==byteValue)
				{
					return; //dont send same value
				}
				pDevice->intvalue=nintvalue;
			}
			else if (vLabel=="Open")
			{
				pDevice->intvalue=255;
			}
			else if (vLabel=="Close")
			{
				pDevice->intvalue=0;
			}
			else
			{
				if ((vLabel!="Switch")&&(vLabel!="Sensor"))
					return;
				if (vType!=OpenZWave::ValueID::ValueType_Bool)
					return;
				int intValue=0;
				if (bValue==true)
					intValue=255;
				else
					intValue=0;
				if (pDevice->intvalue==intValue)
				{
					return; //dont send same value
				}
				pDevice->intvalue=intValue;
			}
		}
		break;
	case ZDTYPE_SWITCHDIMMER:
		{
			if (vLabel!="Level")
				return;
			if (vType!=OpenZWave::ValueID::ValueType_Byte)
				return;
			if (byteValue==99)
				byteValue=255;
			if (pDevice->intvalue==byteValue)
			{
				return; //dont send same value
			}
			pDevice->intvalue=byteValue;
		}
		break;
	case ZDTYPE_SENSOR_POWER:
		if (
			(vLabel!="Energy")&&
			(vLabel!="Power")
			)
			return;
		if (vType!=OpenZWave::ValueID::ValueType_Decimal)
			return;
		pDevice->floatValue=fValue*pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_POWERENERGYMETER:
		if (vType!=OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (
			(vLabel!="Energy")&&
			(vLabel!="Power")
			)
			return;
		pDevice->floatValue=fValue*pDevice->scaleMultiply;
		break;
	case ZDTYPE_SENSOR_TEMPERATURE:
		if (vType!=OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel!="Temperature")
			return;
		if (vUnits=="F")
		{
			//Convert to celcius
			fValue=float((fValue-32)*(5.0/9.0));
		}
		pDevice->bValidValue=(abs(pDevice->floatValue-fValue)<10);
		pDevice->floatValue=fValue;
		break;
	case ZDTYPE_SENSOR_HUMIDITY:
		if (vType!=OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel!="Relative Humidity")
			return;
		pDevice->floatValue=fValue;
		break;
	case ZDTYPE_SENSOR_LIGHT:
		if (vType!=OpenZWave::ValueID::ValueType_Decimal)
			return;
		if (vLabel!="Luminance")
			return;
		pDevice->floatValue=fValue;
		break;
	}

	pDevice->lastreceived=atime;
	pDevice->sequence_number+=1;
	if (pDevice->sequence_number==0)
		pDevice->sequence_number=1;
	if (pDevice->bValidValue)
		SendDevice2Domoticz(pDevice);

}

bool COpenZWave::IncludeDevice()
{
	if (m_pManager==NULL)
		return false;
	m_LastIncludedNode=0;
	m_ControllerCommandStartTime=mytime(NULL);
	m_bControllerCommandInProgress=true;
	m_pManager->BeginControllerCommand(m_controllerID, OpenZWave::Driver::ControllerCommand_AddDevice, OnDeviceStatusUpdate, this, true);

	return true;
}

bool COpenZWave::ExcludeDevice(const int homeID)
{
	if (m_pManager==NULL)
		return false;

	m_ControllerCommandStartTime=mytime(NULL);
	m_bControllerCommandInProgress=true;
	m_pManager->BeginControllerCommand(m_controllerID, OpenZWave::Driver::ControllerCommand_RemoveDevice, OnDeviceStatusUpdate, this, true);

	return true;
}

bool COpenZWave::SoftResetDevice()
{
	if (m_pManager==NULL)
		return false;

	m_pManager->SoftReset(m_controllerID);
	return true;
}

bool COpenZWave::HardResetDevice()
{
	if (m_pManager==NULL)
		return false;

	m_pManager->ResetController(m_controllerID);

	return true;
}

bool COpenZWave::HealNetwork()
{
	if (m_pManager==NULL)
		return false;
	
	m_pManager->HealNetwork(m_controllerID,true);

	return true;
}

bool COpenZWave::RemoveFailedDevice(const int nodeID)
{
	if (m_pManager==NULL)
		return false;

	m_ControllerCommandStartTime=mytime(NULL);
	m_bControllerCommandInProgress=true;
	m_pManager->BeginControllerCommand(m_controllerID, OpenZWave::Driver::ControllerCommand_RemoveFailedNode, OnDeviceStatusUpdate, this,true,(unsigned char)nodeID);

	return true;
}

bool COpenZWave::CancelControllerCommand()
{
	if (m_bControllerCommandInProgress==false)
		return false;
	if (m_pManager==NULL)
		return false;
	m_bControllerCommandInProgress=false;
	return m_pManager->CancelControllerCommand(m_controllerID);
}


void COpenZWave::OnZWaveDeviceStatusUpdate(int _cs, int _err)
{
	OpenZWave::Driver::ControllerState cs=(OpenZWave::Driver::ControllerState)_cs;
	OpenZWave::Driver::ControllerError err=(OpenZWave::Driver::ControllerError)_err;

	std::string szLog;

	switch (cs)
	{
	case OpenZWave::Driver::ControllerState_Normal:
		m_bControllerCommandInProgress=false;
		szLog="No Command in progress";
		break;
	case OpenZWave::Driver::ControllerState_Starting:
		szLog="Starting controller command";
		break;
	case OpenZWave::Driver::ControllerState_Cancel:
		szLog="The command was canceled";
		m_bControllerCommandInProgress=false;
		break;
	case OpenZWave::Driver::ControllerState_Error:
		szLog="Command invocation had error(s) and was aborted";
		m_bControllerCommandInProgress=false;
		break;
	case OpenZWave::Driver::ControllerState_Waiting:
		m_bControllerCommandInProgress=true;
		szLog="Controller is waiting for a user action";
		break;
	case OpenZWave::Driver::ControllerState_Sleeping:
		szLog="Controller command is on a sleep queue wait for device";
		break;
	case OpenZWave::Driver::ControllerState_InProgress:
		szLog="The controller is communicating with the other device to carry out the command";
		break;
	case OpenZWave::Driver::ControllerState_Completed:
		szLog="The command has completed successfully";
		break;
	case OpenZWave::Driver::ControllerState_Failed:
		szLog="The command has failed";
		m_bControllerCommandInProgress=false;
		break;
	case OpenZWave::Driver::ControllerState_NodeOK:
		szLog="Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK";
		break;
	case OpenZWave::Driver::ControllerState_NodeFailed:
		szLog="Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed";
		break;
	default:
		szLog="Unknown Device Response!";
		m_bControllerCommandInProgress=false;
		break;
	}
	_log.Log(LOG_NORM,"OpenZWave: Device Response: %s",szLog.c_str());
}

void COpenZWave::EnableNodePoll(const int homeID, const int nodeID, const int pollTime)
{
	NodeInfo *pNode=GetNodeInfo(homeID, nodeID);
	if (pNode==NULL)
		return; //Not found

	bool bSingleIndexPoll=false;

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ProductDescription FROM ZWaveNodes WHERE (HardwareID==" << m_HwdID << ") AND (HomeID==" << homeID << ") AND (NodeID==" << nodeID << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::string ProductDescription=result[0][0];
		bSingleIndexPoll=(
			(ProductDescription.find("GreenWave PowerNode 6 port")!=std::string::npos)
			);
	}

	for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance=pNode->Instances.begin(); ittInstance!=pNode->Instances.end(); ++ittInstance)
	{

		for( std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds )
		{
			for( std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue!= ittCmds->second.Values.end(); ++ittValue)
			{
				unsigned char commandclass=ittValue->GetCommandClassId();
				OpenZWave::ValueID::ValueGenre vGenre=ittValue->GetGenre();

				//Ignore non-user types
				if (vGenre!=OpenZWave::ValueID::ValueGenre_User)
					continue;

				std::string vLabel=m_pManager->GetValueLabel(*ittValue);

				if (
					(vLabel=="Exporting")||
					(vLabel=="Interval")||
					(vLabel=="Previous Reading")
					)
					continue;;

				if (commandclass==COMMAND_CLASS_SWITCH_BINARY)
				{
					if ((vLabel=="Switch")||(vLabel=="Sensor"))
					{
						m_pManager->EnablePoll(*ittValue,1);
					}
				}
				else if (commandclass==COMMAND_CLASS_SWITCH_MULTILEVEL)
				{
					if (vLabel=="Level")
					{
						m_pManager->EnablePoll(*ittValue,1);
					}
				}
				else if (commandclass==COMMAND_CLASS_SENSOR_BINARY)
				{
					if ((vLabel=="Switch")||(vLabel=="Sensor"))
					{
						m_pManager->EnablePoll(*ittValue,1);
					}
				}
				else if (commandclass==COMMAND_CLASS_METER)
				{
					//Meter device
					if (
						(vLabel=="Energy")||
						(vLabel=="Power")
						)
					{
						if (bSingleIndexPoll&&(ittValue->GetIndex()!=0))
							continue;
						m_pManager->EnablePoll(*ittValue,1);
					}
				}
				else if (commandclass==COMMAND_CLASS_SENSOR_MULTILEVEL)
				{
					if (vLabel=="Temperature")
					{
						m_pManager->EnablePoll(*ittValue,2);
					}
					else if (vLabel=="Luminance")
					{
						m_pManager->EnablePoll(*ittValue,2);
					}
					else if (vLabel=="Relative Humidity")
					{
						m_pManager->EnablePoll(*ittValue,2);
					}
					else if (
						(vLabel=="Energy")||
						(vLabel=="Power")
						)
					{
						if (bSingleIndexPoll&&(ittValue->GetIndex()!=0))
							continue;
						m_pManager->EnablePoll(*ittValue,1);
					}
				}
				else if (commandclass==COMMAND_CLASS_BATTERY)
				{
					m_pManager->EnablePoll(*ittValue,2);
				}
				else
					m_pManager->DisablePoll(*ittValue);
			}
		}
	}
}

void COpenZWave::DisableNodePoll(const int homeID, const int nodeID)
{
	NodeInfo *pNode=GetNodeInfo(homeID, nodeID);
	if (pNode==NULL)
		return; //Not found

	for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance=pNode->Instances.begin(); ittInstance!=pNode->Instances.end(); ++ittInstance)
	{
		for( std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds )
		{
			for( std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue!= ittCmds->second.Values.end(); ++ittValue)
			{
				if (m_pManager->isPolled(*ittValue))
					m_pManager->DisablePoll(*ittValue);
			}
		}
	}
}

void COpenZWave::AddNode(const int homeID, const int nodeID,const NodeInfo *pNode)
{
	//Check if node already exist
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT ID FROM ZWaveNodes WHERE (HardwareID==" << m_HwdID << ") AND (HomeID==" << homeID << ") AND (NodeID==" << nodeID << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	szQuery.clear();
	szQuery.str("");

	std::string sProductDescription=pNode->Manufacturer_name +" " + pNode->Product_name;

	if (result.size()<1)
	{
		//Not Found, Add it to the database
		if (nodeID>1)
			szQuery << "INSERT INTO ZWaveNodes (HardwareID, HomeID, NodeID, ProductDescription) VALUES (" << m_HwdID << "," << homeID << "," << nodeID << ",'" << sProductDescription << "')";
		else
			szQuery << "INSERT INTO ZWaveNodes (HardwareID, HomeID, NodeID, Name,ProductDescription) VALUES (" << m_HwdID << "," << homeID << "," << nodeID << ",'Controller','" << sProductDescription << "')";
	}
	else
	{
		//Update ProductDescription
		szQuery << "UPDATE ZWaveNodes SET ProductDescription='" <<  sProductDescription << "' WHERE (HardwareID==" << m_HwdID << ") AND (HomeID==" << homeID << ") AND (NodeID==" << nodeID << ")";
	}
	m_pMainWorker->m_sql.query(szQuery.str());
}

void COpenZWave::EnableDisableNodePolling()
{
	int intervalseconds=60;
	m_pMainWorker->m_sql.GetPreferencesVar("ZWavePollInterval", intervalseconds);

	m_pManager->SetPollInterval(intervalseconds*1000,false);

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;
	szQuery << "SELECT HomeID,NodeID,PollTime FROM ZWaveNodes WHERE (HardwareID==" << m_HwdID << ")";
	result=m_pMainWorker->m_sql.query(szQuery.str());
	if (result.size()<1)
		return;

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt=result.begin(); itt!=result.end(); ++itt)
	{
		std::vector<std::string> sd=*itt;
		int HomeID=atoi(sd[0].c_str());
		int NodeID=atoi(sd[1].c_str());
		int PollTime=atoi(sd[2].c_str());

		if (
			(HomeID==m_controllerID)&&
			(NodeID>1)
			)
		{
			if (PollTime>0)
				EnableNodePoll(HomeID,NodeID,PollTime);
			else
				DisableNodePoll(HomeID,NodeID);
		}
	}
}

void COpenZWave::NodesQueried()
{
	//All nodes have been queried, enable/disable node polling
	EnableDisableNodePolling();
}

void COpenZWave::GetNodeValuesJson(const int homeID, const int nodeID, Json::Value &root, const int index)
{
	NodeInfo *pNode=GetNodeInfo(homeID, nodeID);
	if (pNode==NULL)
		return;

	int ivalue=0;

	if (nodeID==1)
	{
		//Main ZWave node

		//Poll Interval
		root["result"][index]["config"][ivalue]["type"]="short";

		int intervalseconds=60;
		m_pMainWorker->m_sql.GetPreferencesVar("ZWavePollInterval", intervalseconds);
		root["result"][index]["config"][ivalue]["value"]=intervalseconds;

		root["result"][index]["config"][ivalue]["index"]=1;
		root["result"][index]["config"][ivalue]["label"]="Poll Interval";
		root["result"][index]["config"][ivalue]["units"]="Seconds";
		root["result"][index]["config"][ivalue]["help"]=
			"Set the time period between polls of a node's state. The length of the interval is the same for all devices. "
			"To even out the Z-Wave network traffic generated by polling, OpenZWave divides the polling interval by the number of devices that have polling enabled, and polls each "
			"in turn. It is recommended that if possible, the interval should not be set shorter than the number of polled devices in seconds (so that the network does not have to cope with more than one poll per second).";
		ivalue++;
		//Debug
		root["result"][index]["config"][ivalue]["type"]="short";

		int debugenabled=0;
		m_pMainWorker->m_sql.GetPreferencesVar("ZWaveEnableDebug", debugenabled);
		root["result"][index]["config"][ivalue]["value"]=debugenabled;

		root["result"][index]["config"][ivalue]["index"]=2;
		root["result"][index]["config"][ivalue]["label"]="Enable Debug";
		root["result"][index]["config"][ivalue]["units"]="";
		root["result"][index]["config"][ivalue]["help"]=
			"Enable/Disable debug logging. Disabled=0, Enabled=1 "
			"It is not recommended to enable Debug for a live system as the log files generated will grow large quickly.";

		// convert now to string form
		time_t now=time(NULL);
		char *szDate = asctime(localtime(&now));
		root["result"][index]["config"][ivalue]["LastUpdate"]=szDate;

		ivalue++;
		return;
	}

	for (std::map<int, std::map<int, NodeCommandClass> >::const_iterator ittInstance=pNode->Instances.begin(); ittInstance!=pNode->Instances.end(); ++ittInstance)
	{
		for( std::map<int, NodeCommandClass>::const_iterator ittCmds = ittInstance->second.begin(); ittCmds != ittInstance->second.end(); ++ittCmds )
		{
			for( std::list<OpenZWave::ValueID>::const_iterator ittValue = ittCmds->second.Values.begin(); ittValue!= ittCmds->second.Values.end(); ++ittValue)
			{
				unsigned char commandclass=ittValue->GetCommandClassId();
				if (commandclass==COMMAND_CLASS_CONFIGURATION)
				{
					if (m_pManager->IsValueReadOnly(*ittValue)==true)
						continue;

					std::string szValue;

					OpenZWave::ValueID::ValueType vType=ittValue->GetType();

					if (vType== OpenZWave::ValueID::ValueType_Decimal)
					{
						root["result"][index]["config"][ivalue]["type"]="float";
					}
					else if (vType== OpenZWave::ValueID::ValueType_Bool)
					{
						root["result"][index]["config"][ivalue]["type"]="bool";
					}
					else if (vType== OpenZWave::ValueID::ValueType_Byte)
					{
						root["result"][index]["config"][ivalue]["type"]="byte";
					}
					else if (vType== OpenZWave::ValueID::ValueType_Short)
					{
						root["result"][index]["config"][ivalue]["type"]="short";
					}
					else if (vType== OpenZWave::ValueID::ValueType_Int)
					{
						root["result"][index]["config"][ivalue]["type"]="int";
					}
					else if (vType== OpenZWave::ValueID::ValueType_Button)
					{
						//root["result"][index]["config"][ivalue]["type"]="button";
						//Not supported now
						continue;
					}
					else if (vType== OpenZWave::ValueID::ValueType_List)
					{
						root["result"][index]["config"][ivalue]["type"]="list";
					}
					else
					{
						//not supported
						continue;
					}

					if (m_pManager->GetValueAsString(*ittValue,&szValue)==false)
						continue;
					root["result"][index]["config"][ivalue]["value"]=szValue;
					if (vType== OpenZWave::ValueID::ValueType_List)
					{
						std::vector<std::string> strs;
						m_pManager->GetValueListItems(*ittValue, &strs);
						root["result"][index]["config"][ivalue]["list_items"]=(int)strs.size();
						int vcounter=0;
						for (std::vector<std::string>::const_iterator it = strs.begin(); it != strs.end(); ++it) 
						{
							root["result"][index]["config"][ivalue]["listitem"][vcounter++]=*it;
						}
					}
					int i_index=ittValue->GetIndex();
					std::string i_label=m_pManager->GetValueLabel(*ittValue);
					std::string i_units=m_pManager->GetValueUnits(*ittValue);
					std::string i_help=m_pManager->GetValueHelp(*ittValue);
					char *szDate = asctime(localtime(&ittCmds->second.m_LastSeen));
					root["result"][index]["config"][ivalue]["index"]=i_index;
					root["result"][index]["config"][ivalue]["label"]=i_label;
					root["result"][index]["config"][ivalue]["units"]=i_units;
					root["result"][index]["config"][ivalue]["help"]=i_help;
					root["result"][index]["config"][ivalue]["LastUpdate"]=szDate;
					ivalue++;
				}
			}
		}
	}

}

bool COpenZWave::RequestNodeConfig(const int homeID, const int nodeID)
{
	NodeInfo *pNode=GetNodeInfo( homeID, nodeID);
	if (pNode==NULL)
		return false;
	m_pManager->RequestAllConfigParams(homeID,nodeID);
	return true;
}

bool COpenZWave::ApplyNodeConfig(const int homeID, const int nodeID, const std::string &svaluelist)
{
	NodeInfo *pNode=GetNodeInfo( homeID, nodeID);
	if (pNode==NULL)
		return false;

	std::vector<std::string> results;
	StringSplit(svaluelist,"_",results);
	if (results.size()<1)
		return false;

	size_t vindex=0;
	while (vindex<results.size())
	{
		int rvIndex=atoi(results[vindex].c_str());
		if (nodeID==1)
		{
			//Main ZWave node
			if (rvIndex==1)
			{
				//PollInterval
				int intervalseconds=atoi(results[vindex+1].c_str());
				m_pMainWorker->m_sql.UpdatePreferencesVar("ZWavePollInterval", intervalseconds);
				EnableDisableNodePolling();
			}
			else if (rvIndex==2)
			{
				int debugenabled=atoi(results[vindex+1].c_str());
				int old_debugenabled=0;
				m_pMainWorker->m_sql.GetPreferencesVar("ZWaveEnableDebug", old_debugenabled);
				if (old_debugenabled!=debugenabled)
				{
					m_pMainWorker->m_sql.UpdatePreferencesVar("ZWaveEnableDebug", debugenabled);
					//Restart
					OpenSerialConnector();
				}
			}
		}
		else
		{
			OpenZWave::ValueID vID(0,0,OpenZWave::ValueID::ValueGenre_Basic,0,0,0,OpenZWave::ValueID::ValueType_Bool);

			if (GetNodeConfigValueByIndex(pNode, rvIndex, vID))
			{
				std::string vstring;
				m_pManager->GetValueAsString(vID,&vstring);

				OpenZWave::ValueID::ValueType vType=vID.GetType();
				if (vType==OpenZWave::ValueID::ValueType_List)
				{
					if (vstring!=results[vindex+1])
					{
						m_pManager->SetValueListSelection(vID,results[vindex+1]);
					}
				}
				else
				{
					if (vstring!=results[vindex+1])
					{
						m_pManager->SetValue(vID,results[vindex+1]);
					}
				}
			}
		}
		vindex+=2;
	}
	return true;

}

#endif //WITH_OPENZWAVE

