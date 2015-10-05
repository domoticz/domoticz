#pragma once

#include "DomoticzHardware.h"

#include <string>
#include <vector>
#include "../json/json.h"

class CLogitechMediaServer : public CDomoticzHardwareBase
{
	struct LogitechMediaServerNode
	{
		int				ID;
		int				DevID;
		char			szDevID[40];
		std::string		Name;
		std::string		IP;
		int				Port;
		time_t			LastOK;
		_eMediaStatus	nStatus;
		std::string		sStatus;
		std::string		sShortStatus;
		//		boost::mutex	mutDevice;
	};
public:
	CLogitechMediaServer(const int ID, const std::string IPAddress, const int Port, const int PollIntervalsec, const int PingTimeoutms);
	CLogitechMediaServer(const int ID, const std::string IPAddress, const int Port);
	CLogitechMediaServer(const int ID);
	~CLogitechMediaServer(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
	void SendCommand(const int ID, const std::string &command);
private:
	void Do_Work();
	void GetPlayerInfo();
	void InsertUpdatePlayer(const std::string &Name, const std::string &IPAddress, const int Port);

	bool StartHardware();
	bool StopHardware();
	Json::Value Query(std::string sIP, int iPort, std::string sPostdata);
	void Do_Node_Work(const LogitechMediaServerNode &Node);
	void UpdateNodeStatus(const LogitechMediaServerNode &Node, const _eMediaStatus nStatus, const std::string sStatus, const bool bPingOK);
	void ReloadNodes();

	std::vector<LogitechMediaServerNode> m_nodes;

	int m_iThreadsRunning;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::string	m_IP;
	int	m_Port;
	bool m_bShowedUnsupported;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
};
