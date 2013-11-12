#include "stdafx.h"
#include "OpenZWave.h"

#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <vector>
#include <ctype.h>

#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/Logger.h"
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
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Return the NodeInfo object associated with this notification
//-----------------------------------------------------------------------------
COpenZWave::NodeInfo* COpenZWave::GetNodeInfo( OpenZWave::Notification const* _notification )
{
	unsigned long const homeId = _notification->GetHomeId();
	unsigned char const nodeId = _notification->GetNodeId();
	for( std::list<NodeInfo*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
	{
		NodeInfo* nodeInfo = *it;
		if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
		{
			return nodeInfo;
		}
	}

	return NULL;
}

COpenZWave::NodeInfo* COpenZWave::GetNodeInfo( const int homeID, const int nodeID)
{
	for( std::list<NodeInfo*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
	{
		NodeInfo* nodeInfo = *it;
		if( ( nodeInfo->m_homeId == homeID ) && ( nodeInfo->m_nodeId == nodeID ) )
		{
			return nodeInfo;
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

	OpenZWave::ValueID vID=_notification->GetValueID();

	switch( _notification->GetType() )
	{
	case OpenZWave::Notification::Type_ValueAdded:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Add the new value to our list
				nodeInfo->m_values.push_back( vID );
				nodeInfo->m_LastSeen=mytime(NULL);
				AddValue(vID);
			}
			break;
		}

	case OpenZWave::Notification::Type_ValueRemoved:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Remove the value from out list
				for( std::list<OpenZWave::ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it )
				{
					if( (*it) == vID )
					{
						nodeInfo->m_values.erase( it );
						nodeInfo->m_LastSeen=mytime(NULL);
						break;
					}
				}
			}
			break;
		}

	case OpenZWave::Notification::Type_ValueChanged:
		{
			// One of the node values has changed
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_LastSeen=mytime(NULL);
				UpdateValue(vID);
			}
			break;
		}
	case OpenZWave::Notification::Type_ValueRefreshed:
		{
			// One of the node values has changed
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				UpdateValue(vID);
			}
			break;
		}

	case OpenZWave::Notification::Type_Group:
		{
			// One of the node's association groups has changed
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo = nodeInfo;		// placeholder for real action
			}
			break;
		}

	case OpenZWave::Notification::Type_NodeAdded:
		{
			// Add the new node to our list
			NodeInfo* nodeInfo = new NodeInfo();
			nodeInfo->m_homeId = _notification->GetHomeId();
			nodeInfo->m_nodeId = _notification->GetNodeId();
			nodeInfo->m_polled = false;
			nodeInfo->m_LastSeen=mytime(NULL);
			m_nodes.push_back( nodeInfo );
			break;
		}

	case OpenZWave::Notification::Type_NodeRemoved:
		{
			// Remove the node from our list
			unsigned long const homeId = _notification->GetHomeId();
			unsigned char const nodeId = _notification->GetNodeId();
			for( std::list<NodeInfo*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it )
			{
				NodeInfo* nodeInfo = *it;
				if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
				{
					m_nodes.erase( it );
					delete nodeInfo;
					break;
				}
			}
			break;
		}

	case OpenZWave::Notification::Type_NodeEvent:
		{
			// We have received an event from the node, caused by a
			// basic_set or hail message.
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo = nodeInfo;		// placeholder for real action
			}
			break;
		}

	case OpenZWave::Notification::Type_PollingDisabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = false;
			}
			break;
		}

	case OpenZWave::Notification::Type_PollingEnabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = true;
			}
			break;
		}

	case OpenZWave::Notification::Type_DriverReady:
		{
			m_controllerID = _notification->GetHomeId();
			_log.Log(LOG_NORM,"OpenZWave: Driver Ready");
			break;
		}

	case OpenZWave::Notification::Type_DriverFailed:
		{
			m_initFailed = true;
			_log.Log(LOG_ERROR,"OpenZWave: Driver Failed!!");
			break;
		}

	case OpenZWave::Notification::Type_AwakeNodesQueried:
	case OpenZWave::Notification::Type_AllNodesQueried:
	case OpenZWave::Notification::Type_AllNodesQueriedSomeDead:
		{
			m_nodesQueried = true;
			OpenZWave::Manager::Get()->WriteConfig( m_controllerID );
			//IncludeDevice();
			break;
		}
	case OpenZWave::Notification::Type_NodeNaming:
		{
			break;
		}
	case OpenZWave::Notification::Type_DriverReset:
	case OpenZWave::Notification::Type_NodeProtocolInfo:
	case OpenZWave::Notification::Type_NodeQueriesComplete:
	default:
		{
		}
	}
}

void COpenZWave::StopHardwareIntern()
{
	CloseSerialConnector();
}

bool COpenZWave::OpenSerialConnector()
{
	CloseSerialConnector();
	std::string ConfigPath=szStartupFolder + "Config/";
	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	OpenZWave::Options::Create( ConfigPath.c_str(), ConfigPath.c_str(), "--SaveConfiguration=true " );
#ifdef _DEBUG
	OpenZWave::Options::Get()->AddOptionInt( "SaveLogLevel", OpenZWave::LogLevel_Detail );
	OpenZWave::Options::Get()->AddOptionInt( "QueueLogLevel", OpenZWave::LogLevel_Debug );
	OpenZWave::Options::Get()->AddOptionInt( "DumpTrigger", OpenZWave::LogLevel_Error );
#else
	OpenZWave::Options::Get()->AddOptionInt( "SaveLogLevel", OpenZWave::LogLevel_Error );
	OpenZWave::Options::Get()->AddOptionInt( "QueueLogLevel", OpenZWave::LogLevel_Error );
	OpenZWave::Options::Get()->AddOptionInt( "DumpTrigger", OpenZWave::LogLevel_Error );
#endif
	OpenZWave::Options::Get()->AddOptionInt( "PollInterval", 500 );
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

	OpenZWave::Manager::Get()->AddDriver( m_szSerialPort.c_str() );
	//Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );
	return true;
}

void COpenZWave::CloseSerialConnector()
{
	// program exit (clean up)
	boost::lock_guard<boost::mutex> l(m_NotificationMutex);

	OpenZWave::Manager* pManager=OpenZWave::Manager::Get();
	if (pManager)
	{
		OpenZWave::Manager::Destroy();
		OpenZWave::Options::Destroy();
		_log.Log(LOG_NORM,"OpenZWave: Closed");
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
	return true;
}

bool COpenZWave::GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID &nValue)
{
	COpenZWave::NodeInfo *pNode=GetNodeInfo( m_controllerID, nodeID);
	if (!pNode)
		return false;
	for( std::list<OpenZWave::ValueID>::iterator itt = pNode->m_values.begin(); itt != pNode->m_values.end(); ++itt )
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

void COpenZWave::SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value)
{
	if (m_pManager==NULL)
		return;
	boost::lock_guard<boost::mutex> l(m_NotificationMutex);

	OpenZWave::ValueID vID(0,0,OpenZWave::ValueID::ValueGenre_Basic,0,0,0,OpenZWave::ValueID::ValueType_Bool);

	unsigned char svalue=(unsigned char)value;

	bool bIsDimmer=(GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_SWITCH_MULTILEVEL,vID)==true);
	if (bIsDimmer==false)
	{
		if (GetValueByCommandClass(nodeID, instanceID, COMMAND_CLASS_SWITCH_BINARY,vID)==true)
		{
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
		if (svalue>99)
			svalue=99;
		if (!m_pManager->SetValue(vID,svalue))
		{
			_log.Log(LOG_ERROR,"OpenZWave: Error setting Switch Value!");
		}
	}
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
}

void COpenZWave::AddValue(const OpenZWave::ValueID vID)
{
	if (m_pManager==NULL)
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

	unsigned char instance=vID.GetInstance();
	OpenZWave::ValueID::ValueType vType=vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre=vID.GetGenre();

	//Ignore non-user types
	if (vGenre!=OpenZWave::ValueID::ValueGenre_User)
		return;

	unsigned char NodeID = vID.GetNodeId();
	_log.Log(LOG_NORM, "Value_Added: Node: %d, CommandClass: %s",NodeID, cclassStr(commandclass));

	_tZWaveDevice _device;
	_device.nodeID=NodeID;
	_device.commandClassID=commandclass;
	_device.scaleID=-1;
	_device.instanceID=instance;
	_device.hasWakeup=m_pManager->IsNodeAwake(m_controllerID,NodeID);
	_device.isListening=m_pManager->IsNodeListeningDevice(m_controllerID,NodeID);

	if ((_device.instanceID==0)&&(NodeID==m_controllerID))
		return;// We skip instance 0 if there are more, since it should be mapped to other instances or their superposition

	std::string vLabel=m_pManager->GetValueLabel(vID);
	std::string vUnits=m_pManager->GetValueUnits(vID);


	float fValue=0;
	int iValue=0;
	bool bValue=false;
	unsigned char byteValue=0;

	// We choose SwitchMultilevel first, if not available, SwhichBinary is chosen
	if (commandclass==COMMAND_CLASS_SWITCH_BINARY)
	{
		if (vLabel=="Switch")
		{
			if (m_pManager->GetValueAsBool(vID,&bValue)==true)
			{
				_device.devType= ZDTYPE_SWITCHNORMAL;
				if (bValue==true)
					_device.intvalue=255;
				else
					_device.intvalue=0;
				InsertOrUpdateDevice(_device,false);
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
				InsertOrUpdateDevice(_device,false);
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
					InsertOrUpdateDevice(_device,false);
				}
			}
		}
	}
	else
	{
		//Unhanded
		_log.Log(LOG_NORM, "^Unhanded^");
	}
}

void COpenZWave::UpdateValue(const OpenZWave::ValueID vID)
{
	if (m_nodesQueried==false)
		return; //only allow updates when node Query is done
	unsigned char commandclass=vID.GetCommandClassId();
	unsigned char instance=vID.GetInstance();
	OpenZWave::ValueID::ValueType vType=vID.GetType();
	OpenZWave::ValueID::ValueGenre vGenre=vID.GetGenre();
	unsigned char NodeID = vID.GetNodeId();
	std::string vLabel=m_pManager->GetValueLabel(vID);
	std::string vUnits=m_pManager->GetValueUnits(vID);

	time_t atime=mytime(NULL);
	std::stringstream sstr;
	sstr << int(NodeID) << ".instances." << int(instance) << ".commandClasses." << int(commandclass) << ".data";
	std::string path=sstr.str();

	_log.Log(LOG_NORM, "Value_Changed: Node: %d, CommandClass: %s",NodeID, cclassStr(commandclass));

	if (commandclass==128)
	{
		//Battery status update
		return;
	}
	float fValue=0;
	int iValue=0;
	bool bValue=false;
	unsigned char byteValue=0;

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
		return;

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
	switch (pDevice->devType)
	{
	case ZDTYPE_SWITCHNORMAL:
		{
			if (vLabel!="Switch")
				return;
			if (vType!=OpenZWave::ValueID::ValueType_Bool)
				return;
			int intValue=0;
			if (bValue==true)
				intValue=255;
			else
				intValue=0;
			pDevice->intvalue=intValue;
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
	}

	pDevice->lastreceived=atime;
	pDevice->sequence_number+=1;
	if (pDevice->sequence_number==0)
		pDevice->sequence_number=1;
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

bool COpenZWave::ExcludeDevice(const int nodeID)
{
	if (m_pManager==NULL)
		return false;

	m_ControllerCommandStartTime=mytime(NULL);
	m_bControllerCommandInProgress=true;
	m_pManager->BeginControllerCommand(m_controllerID, OpenZWave::Driver::ControllerCommand_RemoveDevice, OnDeviceStatusUpdate, this, true);

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
		szLog="No Command in progress";
		break;
	case OpenZWave::Driver::ControllerState_Starting:
		szLog="Starting controller command";
		break;
	case OpenZWave::Driver::ControllerState_Cancel:
		szLog="The command was canceled";
		break;
	case OpenZWave::Driver::ControllerState_Error:
		szLog="Command invocation had error(s) and was aborted";
		break;
	case OpenZWave::Driver::ControllerState_Waiting:
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
		break;
	case OpenZWave::Driver::ControllerState_NodeOK:
		szLog="Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node is OK";
		break;
	case OpenZWave::Driver::ControllerState_NodeFailed:
		szLog="Used only with ControllerCommand_HasNodeFailed to indicate that the controller thinks the node has failed";
		break;
	default:
		szLog="Unknown Device Response!";
		break;
	}
	_log.Log(LOG_NORM,"Device Response: %s",szLog.c_str());
}
