#pragma once

#ifdef WITH_OPENZWAVE

#include <map>
#include <string>
#include <time.h>
#include "ZWaveBase.h"
#include "ASyncSerial.h"
#include <list>

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

class COpenZWave : public AsyncSerial, public ZWaveBase
{
public:
	typedef struct  
	{
		std::list<OpenZWave::ValueID>	Values;
		time_t							m_LastSeen;
	}NodeCommandClass;

	typedef struct
	{
		unsigned long					m_homeId;
		unsigned char					m_nodeId;
		bool							m_polled;

		std::string						szType;
		int								iVersion;
		std::string						Manufacturer_id;
		std::string						Manufacturer_name;
		std::string						Product_type;
		std::string						Product_id;
		std::string						Product_name;

		std::map<int, std::map<int, NodeCommandClass> >	Instances;

		bool							IsAwake;
		bool							IsDead;

		bool							HaveUserCodes;

		time_t							m_LastSeen;
	}NodeInfo;

	COpenZWave(const int ID, const std::string& devname);
	~COpenZWave(void);

	bool GetInitialDevices();
	bool GetUpdates();
	void OnZWaveNotification( OpenZWave::Notification const* _notification);
	void OnZWaveDeviceStatusUpdate(int cs, int err);
	void EnableDisableNodePolling();
	void GetNodeValuesJson(const int homeID, const int nodeID, Json::Value &root, const int index);
	NodeInfo* GetNodeInfo( const int homeID, const int nodeID );
	bool ApplyNodeConfig(const int homeID, const int nodeID, const std::string &svaluelist);

	bool SetUserCodeEnrollmentMode();
	bool GetNodeUserCodes(const int homeID, const int nodeID, Json::Value &root);
	bool RemoveUserCode(const int homeID, const int nodeID, const int codeIndex);

	//Controller Commands
	bool RequestNodeConfig(const int homeID, const int nodeID);
	bool RemoveFailedDevice(const int nodeID);
	bool ReceiveConfigurationFromOtherController();
	bool SendConfigurationToSecondaryController();
	bool TransferPrimaryRole();
	bool CancelControllerCommand();
	bool IncludeDevice();
	bool ExcludeDevice(const int homeID);
	bool SoftResetDevice();
	bool HardResetDevice();
	bool HealNetwork();
	bool HealNode(const int nodeID);
	bool GetFailedState();
	bool NetworkInfo(const int hwID,std::vector< std::vector< int > > &NodeArray);
	int ListGroupsForNode(const int nodeID);
	int ListAssociatedNodesinGroup(const int nodeID,const int groupID,std::vector< int > &nodesingroup);
	bool AddNodeToGroup(const int nodeID,const int groupID, const int addID);
	bool RemoveNodeFromGroup(const int nodeID,const int groupID, const int removeID);
	std::string GetConfigFile(std::string &szConfigFile);
private:
	void NodesQueried();
	void DeleteNode(const int homeID, const int nodeID);
	void AddNode(const int homeID, const int nodeID,const NodeInfo *pNode);
	void EnableNodePoll(const int homeID, const int nodeID, const int pollTime);
	void DisableNodePoll(const int homeID, const int nodeID);
	bool GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID &nValue);
	bool GetValueByCommandClassLabel(const int nodeID, const int instanceID, const int commandClass, const std::string &vLabel, OpenZWave::ValueID &nValue);
	bool GetNodeConfigValueByIndex(const NodeInfo *pNode, const int index, OpenZWave::ValueID &nValue);
	void AddValue(const OpenZWave::ValueID vID);
	void UpdateValue(const OpenZWave::ValueID vID);
	void UpdateNodeEvent(const OpenZWave::ValueID vID, int EventID);
	void UpdateNodeScene(const OpenZWave::ValueID vID, int SceneID);
	NodeInfo* GetNodeInfo( OpenZWave::Notification const* _notification );
	void SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value);
	void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value);
	bool IsNodeRGBW(const int homeID, const int nodeID);

	void StopHardwareIntern();

	void EnableDisableDebug();

	bool OpenSerialConnector();
	void CloseSerialConnector();

	void WriteControllerConfig();
	time_t m_LastControllerConfigWrite;

	OpenZWave::Manager *m_pManager;

	std::list<NodeInfo> m_nodes;
	boost::mutex m_NotificationMutex;

	std::string m_szSerialPort;
	int m_controllerID;

	bool m_initFailed;
	bool m_awakeNodesQueried;
	bool m_allNodesQueried;
	bool m_bInUserCodeEnrollmentMode;
};

#endif //WITH_OPENZWAVE

