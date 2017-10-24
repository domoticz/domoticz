#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include <iosfwd>

typedef struct _tYouLessMeter {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned short ID1;
	unsigned long powerusage;
	unsigned long usagecurrent;
} YouLessMeter;

class CYouLess : public CDomoticzHardwareBase
{
public:
	CYouLess(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password);
	~CYouLess(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_Password;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	YouLessMeter	m_meter;
	bool m_bCheckP1;
	bool m_bHaveP1;
	P1Power	m_p1power;
	P1Gas	m_p1gas;
	unsigned long m_lastgasusage;
	time_t m_lastSharedSendGas;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
	bool GetP1Details();
};

