#pragma once

#include "DomoticzHardware.h"

#include <string>

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
	CPinger(int ID, int PollIntervalsec, int PingTimeoutms);
	~CPinger() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &IPAddress, int Timeout);
	bool UpdateNode(int ID, const std::string &Name, const std::string &IPAddress, int Timeout);
	void RemoveNode(int ID);
	void RemoveAllNodes();
	void SetSettings(int PollIntervalsec, int PingTimeoutms);

      private:
	void Do_Work();
	bool StartHardware() override;
	bool StopHardware() override;
	void DoPingHosts();
	void Do_Ping_Worker(const PingNode &Node);
	void UpdateNodeStatus(const PingNode &Node, bool bPingOK);
	void ReloadNodes();

      private:
	int m_iThreadsRunning;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::vector<PingNode> m_nodes;
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;
};
