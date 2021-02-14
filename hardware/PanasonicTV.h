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
	CPanasonic(int ID, int PollIntervalsec, int PingTimeoutms, int Mode3);
	explicit CPanasonic(int ID);
	~CPanasonic() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &IPAddress, int Port);
	bool UpdateNode(int ID, const std::string &Name, const std::string &IPAddress, int Port);
	void RemoveNode(int ID);
	void RemoveAllNodes();
	void SetSettings(int PollIntervalsec, int PingTimeoutms, int Mode3);
	void SendCommand(int ID, const std::string &command);
	bool SetExecuteCommand(int ID, const std::string &command);

      private:
	void Do_Work();

	bool StartHardware() override;
	bool StopHardware() override;

	void ReloadNodes();
	void UnloadNodes();

      private:
	static std::vector<std::shared_ptr<CPanasonicNode>> m_pNodes;
	int m_iPollInterval;
	int m_iPingTimeoutms;
	bool m_bUnknownCommandAllowed;
	bool m_bTryIfOff;
	std::shared_ptr<std::thread> m_thread;
	std::mutex m_mutex;
	boost::asio::io_service m_ios;

	friend class CPanasonicNode; 
};
