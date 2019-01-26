#pragma once

#include "DomoticzHardware.h"

#include <string>

namespace Json
{
	class Value;
};

class CLogitechMediaServer : public CDomoticzHardwareBase
{
	struct LogitechMediaServerNode
	{
		int				ID;
		int				DevID;
		char			szDevID[40];
		std::string		Name;
		std::string		IP;
		time_t			LastOK;
		_eMediaStatus	nStatus;
		std::string		sStatus;
		std::string		sShortStatus;
	};
public:
	struct LMSPlaylistNode
	{
		int				ID;
		int				refID;
		std::string		Name;
	};
	CLogitechMediaServer(const int ID, const std::string &IPAddress, const int Port, const std::string &User, const std::string &Pwd, const int PollIntervalsec);
	explicit CLogitechMediaServer(const int ID);
	~CLogitechMediaServer(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec);
	bool SendCommand(const int ID, const std::string &command, const std::string &param = "");
	std::vector<LMSPlaylistNode> GetPlaylists();
	void SendText(const std::string &playerIP, const std::string &subject, const std::string &text, const int duration);
	int GetPlaylistRefID(const std::string &name);
private:
	_eNotificationTypes	NotificationType(_eMediaStatus nStatus);
	void Do_Work();
	void GetPlayerInfo();
	void UpsertPlayer(const std::string &Name, const std::string &IPAddress, const std::string &MacAddress);

	bool StartHardware() override;
	bool StopHardware() override;
	Json::Value Query(const std::string &sIP, const int iPort, const std::string &sPostdata);
	void Do_Node_Work(const LogitechMediaServerNode &Node);
	void UpdateNodeStatus(const LogitechMediaServerNode &Node, const _eMediaStatus nStatus, const std::string &sStatus, const bool bPingOK);
	void ReloadNodes();
	void ReloadPlaylists();
	std::string GetPlaylistByRefID(const int ID);
private:
	std::vector<LogitechMediaServerNode> m_nodes;
	std::vector<LMSPlaylistNode> m_playlists;

	int m_iThreadsRunning;
	int m_iPollInterval;
	std::string	m_IP;
	int	m_Port;
	std::string m_User;
	std::string m_Pwd;
	bool m_bShowedStartupMessage;
	int m_iMissedQueries;

	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;
};
