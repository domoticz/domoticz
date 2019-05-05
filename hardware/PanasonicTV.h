#pragma once

#include "DomoticzHardware.h"

#include "../main/localtime_r.h"
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>

class CPanasonicNode;

class CPanasonic : public CDomoticzHardwareBase
{
public:
	CPanasonic(const int ID, const int PollIntervalsec, const int PingTimeoutms);
	explicit CPanasonic(const int ID);
	~CPanasonic(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &IPAddress, const int Port);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress, const int Port);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
	void SetSettings(const int PollIntervalsec, const int PingTimeoutms);
	void SendCommand(const int ID, const std::string &command);
	bool SetExecuteCommand(const int ID, const std::string &command);
private:
	void Do_Work();

	bool StartHardware() override;
	bool StopHardware() override;

	void ReloadNodes();
	void UnloadNodes();
private:
	static	std::vector<std::shared_ptr<CPanasonicNode> > m_pNodes;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;
	boost::asio::io_service m_ios;
};
