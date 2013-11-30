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
class COpenZWave : public AsyncSerial, public ZWaveBase
{
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

		bool							m_WasSleeping;
		time_t							m_LastSeen;
	}NodeInfo;

public:
	COpenZWave(const int ID, const std::string& devname);
	~COpenZWave(void);

	bool GetInitialDevices();
	bool GetUpdates();
	void OnZWaveNotification( OpenZWave::Notification const* _notification);
	void OnZWaveDeviceStatusUpdate(int cs, int err);
	void EnableDisableNodePolling();
private:
	void NodesQueried();
	void AddNode(const int homeID, const int nodeID,const NodeInfo *pNode);
	void EnableNodePoll(const int homeID, const int nodeID, const int pollTime);
	void DisableNodePoll(const int homeID, const int nodeID);
	bool GetValueByCommandClass(const int nodeID, const int instanceID, const int commandClass, OpenZWave::ValueID &nValue);
	void AddValue(const OpenZWave::ValueID vID);
	void UpdateValue(const OpenZWave::ValueID vID);
	NodeInfo* GetNodeInfo( OpenZWave::Notification const* _notification );
	NodeInfo* GetNodeInfo( const int homeID, const int nodeID );
	void SwitchLight(const int nodeID, const int instanceID, const int commandClass, const int value);
	void SetThermostatSetPoint(const int nodeID, const int instanceID, const int commandClass, const float value);
	void StopHardwareIntern();
	bool CancelControllerCommand();
	bool IncludeDevice();
	bool ExcludeDevice(const int nodeID);
	bool RemoveFailedDevice(const int nodeID);

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

