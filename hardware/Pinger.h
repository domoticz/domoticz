#pragma once

#include "DomoticzHardware.h"

#include <string>
#include <vector>

class CPinger : public CDomoticzHardwareBase
{
	struct PingNode
	{
		int ID;
		std::string Name;
		std::string IP;
		time_t LastOK;
		int SensorTimeoutSec;
	};
public:
	CPinger(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	~CPinger(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Timeout);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Timeout);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
private:
	void Do_Work();

	bool StartHardware();
	bool StopHardware();
	void DoPingHosts();

	void Do_Ping_Worker(const PingNode &Node);
	void UpdateNodeStatus(const PingNode &Node, const bool bPingOK);

	void ReloadNodes();

	int m_iThreadsRunning;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::vector<PingNode> m_nodes;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
};

