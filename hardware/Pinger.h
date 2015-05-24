#pragma once

#include "DomoticzHardware.h"

class CPinger : public CDomoticzHardwareBase
{
public:
	CPinger(const int ID);
	~CPinger(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void AddNode(const std::string &Name, const std::string &IPAddress);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &IPAddress);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
private:
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	boost::mutex m_mutex;

	bool StartHardware();
	bool StopHardware();
	void DoPingHosts();

	void Do_Work();
};

