#pragma once

#include "DomoticzHardware.h"

#include <string>
#include <vector>
#include "../json/json.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

class CHEOS : public CDomoticzHardwareBase
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

	CHEOS(const int ID, const std::string &IPAddress, const int Port, const std::string &User, const std::string &Pwd, const int PollIntervalsec, const int PingTimeoutms);
	explicit CHEOS(const int ID);
	~CHEOS(void);
	void 	SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void 	Restart();
	void	SendCommand(const std::string&);
	void	SendCommand(const std::string&, const int iValue);

protected:
	bool	m_Busy;
	
private:
	_eNotificationTypes	NotificationType(_eMediaStatus nStatus);

	std::vector<HEOSNode> m_nodes;

	int m_iThreadsRunning;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::string	m_IP;
	int		m_Port;
	std::string m_User;
	std::string m_Pwd;
	bool m_bShowedStartupMessage;
	int m_iMissedQueries;
	std::string m_RetainedData;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;

	void	Do_Work();
	void	handleConnect();
	void	handleRead(const boost::system::error_code&, std::size_t);
	void	handleWrite(std::string);
	void	handleDisconnect();
	void	handleMessage(std::string&);

	bool 	StartHardware();
	bool 	StopHardware();

	void 	AddNode(const std::string &Name, const std::string &PlayerID);
	bool 	UpdateNode(const int ID, const std::string &Name);
	void	UpdateNodeStatus(const HEOSNode &Node, const _eMediaStatus nStatus, const std::string &sStatus);
//	void 	RemoveNode(const int ID);
	void 	ReloadNodes();


};