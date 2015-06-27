#pragma once

#include "DomoticzHardware.h"

#include <string>
#include <vector>

class CKodi : public CDomoticzHardwareBase
{
	struct KodiNode
	{
		int				ID;
		std::string		Name;
		std::string		IP;
		int				Port;
		time_t			LastOK;
		_eMediaStatus	nStatus;
		std::string		sStatus;
	};
public:
	CKodi(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	~CKodi(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
private:
	void Do_Work();

	bool StartHardware();
	bool StopHardware();

	void Do_Node_Work(const KodiNode &Node);
	void UpdateNodeStatus(const KodiNode &Node, const _eMediaStatus nStatus, const std::string sStatus, const bool bPingOK);

	void ReloadNodes();

	int m_iThreadsRunning;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::vector<KodiNode> m_nodes;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
};

