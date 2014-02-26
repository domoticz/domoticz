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
	//Controller Commands
	bool RequestNodeConfig(const int homeID, const int nodeID);
	bool RemoveFailedDevice(const int nodeID);
	bool CancelControllerCommand();
	bool IncludeDevice();
	bool ExcludeDevice(const int homeID);
	bool SoftResetDevice();
	bool HardResetDevice();
	bool HealNetwork();
	bool NetworkInfo(const int hwID,std::vector< std::vector< int > > &NodeArray);
private:
	void NodesQueried();
	void AddNode(const int homeID, const int nodeID,const NodeInfo *pNode);
	void EnableNodePoll(const int homeID, const int nodeID, const int pollTime);
	void DisableNodePoll(const int homeID, const int nodeID);
	bool GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID &nValue);
	bool GetValueByCommandClassLabel(const int nodeID, const int instanceID, const int commandClass, const std::string &vLabel, OpenZWave::ValueID &nValue);
	bool GetNodeConfigValueByIndex(const NodeInfo *pNode, const int index, OpenZWave::ValueID &nValue);
	void AddValue(const OpenZWave::ValueID vID);
	void UpdateValue(const OpenZWave::ValueID vID);
	NodeInfo* GetNodeInfo( OpenZWave::Notification const* _notification );
	void SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value);
	void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value);
	void StopHardwareIntern();

	void EnableDisableDebug();

	bool OpenSerialConnector();
	void CloseSerialConnector();

	OpenZWave::Manager *m_pManager;

	std::list<NodeInfo> m_nodes;
	boost::mutex m_NotificationMutex;

	std::string m_szSerialPort;
	int m_controllerID;

	bool m_initFailed;
	bool m_nodesQueried;
};

#endif //WITH_OPENZWAVE

