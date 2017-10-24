#pragma once

#include "DomoticzHardware.h"

#include "../main/localtime_r.h"
#include <string>
#include <vector>
#include "../json/json.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

class CPanasonicNode;

class CPanasonic : public CDomoticzHardwareBase
{
public:
	CPanasonic(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	explicit CPanasonic(const int ID);
	~CPanasonic(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void Restart();
	void SendCommand(const int ID, const std::string &command);
	bool SetExecuteCommand(const int ID, const std::string &command);
private:
	void Do_Work();

	bool StartHardware();
	bool StopHardware();

	void ReloadNodes();
	void UnloadNodes();

	static	std::vector<boost::shared_ptr<CPanasonicNode> > m_pNodes;

	int m_iPollInterval;
	int m_iPingTimeoutms;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;
	boost::asio::io_service m_ios;
};
