#pragma once

#include <map>
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
		unsigned long			m_homeId;
		unsigned char			m_nodeId;
		bool			m_polled;
		std::list<OpenZWave::ValueID>	m_values;
	}NodeInfo;

public:
	COpenZWave(const int ID, const std::string& devname);
	~COpenZWave(void);

	bool GetInitialDevices();
	bool GetUpdates();
	void OnZWaveNotification( OpenZWave::Notification const* _notification);
private:
	void AddValue(const OpenZWave::ValueID vID);
	void UpdateValue(const OpenZWave::ValueID vID);
	NodeInfo* GetNodeInfo( OpenZWave::Notification const* _notification );
	void RunCMD(const std::string &cmd);
	void StopHardwareIntern();

	bool OpenSerialConnector();
	void CloseSerialConnector();

	OpenZWave::Manager *m_pManager;

	std::list<NodeInfo*> m_nodes;
	boost::mutex m_NotificationMutex;

	std::string m_szSerialPort;
	int m_controllerID;

	bool m_initFailed;
	bool m_nodesQueried;

};



