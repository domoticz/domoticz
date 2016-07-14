#pragma once

// implememtation for devices : http://blebox.eu/
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"
#include "../json/json.h"

class BleBox : public CDomoticzHardwareBase
{
public:
	BleBox(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	virtual ~BleBox();

	bool WriteToHardware(const char *pdata, const unsigned char length);

	void AddNode(const std::string &Name, const std::string &IPAddress);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();

private:
	volatile bool m_stoprequested;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	boost::shared_ptr<boost::thread> m_thread;
	boost::mutex m_mutex;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	std::string IdentifyDevice(const std::string &IPAddress);

	void ReloadNodes();
	void UnloadNodes();
};
