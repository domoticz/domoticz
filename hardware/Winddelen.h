#pragma once

#include "DomoticzHardware.h"

class CWinddelen : public CDomoticzHardwareBase
{
public:
	CWinddelen(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const unsigned short usMillID);
	~CWinddelen(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetMeterDetails();
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	unsigned short m_usMillID;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
};

