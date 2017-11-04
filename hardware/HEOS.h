#pragma once

#include "DomoticzHardware.h"

#include "ASyncTCP.h"
#include "../json/json.h"

#include <iosfwd>
#include <string>
#include <vector>

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
	
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void SendCommand(const std::string&);
	void SendCommand(const std::string&, const int iValue);
	bool isConnected(){ return mIsConnected; };

public:
	// signals
	boost::signals2::signal<void()> sDisconnected;

private:

	_eNotificationTypes	NotificationType(_eMediaStatus nStatus);

	std::vector<HEOSNode> m_nodes;

	int m_retrycntr;
	bool StartHardware();
	bool StopHardware();
	//bool WriteInt(const unsigned char *pData, const unsigned char Len);
	bool WriteInt(const std::string &sendStr);

	int m_lastUpdate;
	
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::string	m_IP;
	std::string m_User;
	std::string m_Pwd;
	
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_bDoRestart;

	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);
	
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	
	void AddNode(const std::string &Name, const std::string &PlayerID);
	void UpdateNode(const int ID, const std::string &Name);
	void UpdateNodeStatus(const std::string &DevID, const _eMediaStatus nStatus, const std::string &sStatus);
	void UpdateNodesStatus(const std::string &DevID, const std::string &sStatus);
//	void RemoveNode(const int ID);
	void ReloadNodes();
	
	unsigned char m_buffer[1028];
	int m_bufferpos;	
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

};