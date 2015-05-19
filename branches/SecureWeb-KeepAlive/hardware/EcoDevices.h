#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CEcoDevices : public CDomoticzHardwareBase
{
public:
	CEcoDevices(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~CEcoDevices(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool Login();
	void Logout();

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	P1Power	m_p1power;
	time_t m_lastSharedSendElectra;
	unsigned long m_lastelectrausage;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

