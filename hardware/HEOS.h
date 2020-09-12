#pragma once

#include "DomoticzHardware.h"
#include "ASyncTCP.h"
#include <string>

class CHEOS : public CDomoticzHardwareBase, ASyncTCP
{
	struct HEOSNode
	{
		int				ID;
		int				DevID;
		std::string		Name;
		time_t			LastOK;
		_eMediaStatus	nStatus;
		std::string		sStatus;
		std::string		sShortStatus;
	};

public:
	CHEOS(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &User, const std::string &Pwd, const int PollIntervalsec, const int PingTimeoutms);
	~CHEOS(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	boost::signals2::signal<void()> sDisconnected;
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void SendCommand(const std::string&, const int iValue);
private:
	void SendCommand(const std::string&);
	_eNotificationTypes	NotificationType(_eMediaStatus nStatus);
	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteInt(const std::string &sendStr);
	void Do_Work();

	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const boost::system::error_code& error) override;

	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	void AddNode(const std::string &Name, const std::string &PlayerID);
	void UpdateNode(const int ID, const std::string &Name);
	void UpdateNodeStatus(const std::string &DevID, const _eMediaStatus nStatus, const std::string &sStatus);
	void UpdateNodesStatus(const std::string &DevID, const std::string &sStatus);
	//	void RemoveNode(const int ID);
	void ReloadNodes();
private:
	std::vector<HEOSNode> m_nodes;
	int m_retrycntr;

	int m_lastUpdate;

	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::string	m_IP;
	std::string m_User;
	std::string m_Pwd;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	unsigned char m_buffer[1028];
	int m_bufferpos;
	std::shared_ptr<std::thread> m_thread;
};