#pragma once

#ifdef WITH_OPENZWAVE

#include <string>
#include <time.h>
#include "ZWaveBase.h"
#include "ASyncSerial.h"
#include <list>
#include "openzwave/control_panel/ozwcp.h"

namespace OpenZWave
{
	class ValueID;
	class Manager;
	class Notification;
}

namespace Json
{
	class Value;
}

class COpenZWave : public ZWaveBase
{
public:
	typedef struct  
	{
		std::list<OpenZWave::ValueID>	Values;
		time_t							m_LastSeen;
	}NodeCommandClass;
	
	typedef enum
	{
		NTSATE_UNKNOWN=0,
		NSTATE_AWAKE,
		NSTATE_SLEEP,
		NSTATE_DEAD
	} eNodeState;

	typedef struct
	{
		unsigned int					homeId;
		uint8_t							nodeId;
		bool							polled;

		std::string						szType;
		int								iVersion;
		uint16_t						Manufacturer_id;
		std::string						Manufacturer_name;
		uint16_t						Product_type;
		uint16_t						Product_id;
		std::string						Product_name;
		int								Application_version;

		int								batValue = 255;

		std::map<uint8_t, std::map<uint8_t, NodeCommandClass> >	Instances; //<instance>/<commandclass>

		eNodeState						eState;

		bool							HaveUserCodes;
		bool							IsPlus;

		time_t							LastSeen;

		//Thermostat settings
		int								tClockDay;
		int								tClockHour;
		int								tClockMinute;
		int								tMode;
		int								tFanMode;
		std::vector<std::string>				tModes;
		std::vector<std::string>				tFanModes;
	}NodeInfo;

	COpenZWave(const int ID, const std::string& devname);
	~COpenZWave(void);

	bool GetInitialDevices();
	bool GetUpdates();
	void OnZWaveNotification( OpenZWave::Notification const* _notification);
	void OnZWaveDeviceStatusUpdate(int cs, int err);
	void EnableDisableNodePolling(const uint8_t nodeID);
	void SetNodeName(const unsigned int homeID, const uint8_t nodeID, const std::string &Name);
	std::string GetNodeStateString(const unsigned int homeID, const uint8_t nodeID);
	std::string GetNodeGenericType(const bool bIsPlus, const uint32 homeID, const uint8 nodeID);
	void GetNodeValuesJson(const unsigned int homeID, const uint8_t nodeID, Json::Value &root, const int index);
	bool ApplyNodeConfig(const unsigned int homeID, const uint8_t nodeID, const std::string &svaluelist);
	NodeInfo* GetNodeInfo(const unsigned int homeID, const uint8_t nodeID);

	std::string GetVersion();
	std::string GetVersionLong();

	bool SetUserCodeEnrollmentMode();
	bool GetNodeUserCodes(const unsigned int homeID, const uint8_t nodeID, Json::Value &root);
	bool RemoveUserCode(const unsigned int homeID, const uint8_t nodeID, const int codeIndex);

	bool GetBatteryLevels(Json::Value& root);

	std::vector<std::string> GetSupportedThermostatModes(const unsigned long ID);
	std::string GetSupportedThermostatFanModes(const unsigned long ID);

	//Controller Commands
	bool RequestNodeConfig(const unsigned int homeID, const uint8_t nodeID);
	bool RequestNodeInfo(const unsigned int homeID, const uint8_t nodeID);
	bool RemoveFailedDevice(const uint8_t nodeID);
	bool HasNodeFailed(const uint8_t nodeID);
	bool ReceiveConfigurationFromOtherController();
	bool SendConfigurationToSecondaryController();
	bool TransferPrimaryRole();
	bool CancelControllerCommand(const bool bForce = false);
	bool IncludeDevice(const bool bSecure);
	bool ExcludeDevice(const uint8_t nodeID);
	bool IsNodeIncluded();
	bool IsNodeExcluded();
	bool SoftResetDevice();
	bool HardResetDevice();
	bool HealNetwork();
	bool HealNode(const uint8_t nodeID);
	bool GetFailedState();
	bool NetworkInfo(const int hwID,std::vector< std::vector< int > > &NodeArray);
	int ListGroupsForNode(const uint8_t nodeID);
	std::string GetGroupName(const uint8_t nodeID, const uint8_t groupID);
	int ListAssociatedNodesinGroup(const uint8_t nodeID,const uint8_t groupID,std::vector< std::string > &nodesingroup);
	bool AddNodeToGroup(const uint8_t nodeID,const uint8_t groupID, const uint8_t addID, const uint8_t instance);
	bool RemoveNodeFromGroup(const uint8_t nodeID,const uint8_t groupID, const uint8_t removeID, const uint8_t instance);
	void GetConfigFile(std::string & filePath, std::string & fileContent);
	unsigned int GetControllerID();
	void NightlyNodeHeal();

	bool m_awakeNodesQueried;
	bool m_allNodesQueried;
	uint8_t m_controllerNodeId;
	COpenZWaveControlPanel m_ozwcp;
private:
	void NodeQueried(const unsigned int homeID, const uint8_t nodeID);
	void DeleteNode(const unsigned int homeID, const uint8_t nodeID);
	void AddNode(const unsigned int homeID, const uint8_t nodeID,const NodeInfo *pNode);
	void EnableNodePoll(const unsigned int homeID, const uint8_t nodeID, const int pollTime);
	void DisableNodePoll(const unsigned int homeID, const uint8_t nodeID);
	bool GetValueByCommandClass(const uint8_t nodeID, const uint8_t instanceID, const uint8_t commandClass, OpenZWave::ValueID &nValue);
	bool GetValueByCommandClassIndex(const uint8_t nodeID, const uint8_t instanceID, const uint8_t commandClass, const uint16_t vIndex, OpenZWave::ValueID &nValue);
	bool GetNodeConfigValueByIndex(const NodeInfo *pNode, const int index, OpenZWave::ValueID &nValue);
	void DebugValue(const OpenZWave::ValueID& vID, const int Line);
	void AddValue(NodeInfo* pNode, const OpenZWave::ValueID &vID);
	void UpdateValue(NodeInfo* pNode, const OpenZWave::ValueID &vID);
	void UpdateNodeEvent(const OpenZWave::ValueID &vID, int EventID);
	bool SwitchLight(_tZWaveDevice* pDevice, const int instanceID, const int value);
	bool SwitchColor(const uint8_t nodeID, const uint8_t instanceID, const std::string &ColorStr);
	void SetThermostatSetPoint(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const float value);
	void SetClock(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int day, const int hour, const int minute);
	void SetThermostatMode(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int tMode);
	void SetThermostatFanMode(const uint8_t nodeID, const uint8_t instanceID, const int commandClass, const int fMode);
	void UpdateDeviceBatteryStatus(const uint8_t nodeID, const int value);
	void ForceUpdateForNodeDevices(const unsigned int homeID, const int nodeID);
	bool IsNodeRGBW(const unsigned int homeID, const int nodeID);

	uint8_t GetInstanceFromValueID(const OpenZWave::ValueID &vID);

	void StopHardwareIntern();

	void EnableDisableDebug();

	bool OpenSerialConnector();
	void CloseSerialConnector();

	OpenZWave::Manager *m_pManager;

	std::list<NodeInfo> m_nodes;

	std::string m_szSerialPort;
	unsigned int m_controllerID;

	bool m_bIsShuttingDown;
	bool m_initFailed;
	bool m_bInUserCodeEnrollmentMode;
	bool m_bNightlyNetworkHeal;
	bool m_bAeotecBlinkingMode;
	int	m_LastAlarmTypeReceived;
};

#endif //WITH_OPENZWAVE

