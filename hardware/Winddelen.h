#pragma once

#include "DomoticzHardware.h"

class CWinddelen : public CDomoticzHardwareBase
{
public:
	CWinddelen(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const unsigned short usMillID);
	~CWinddelen(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	unsigned short m_usMillID;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

