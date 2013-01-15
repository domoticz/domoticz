#pragma once

#include "DomoticzHardware.h"
#include <iostream>

#define pTypeYouLess	0xFC
#define sTypeYouLess	0x01

typedef struct _tYouLessMeter {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	std::string ID;
	unsigned long powerusage;
	unsigned long usagecurrent;
} YouLessMeter;

class CYouLess : public CDomoticzHardwareBase
{
public:
	CYouLess(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~CYouLess(void);

	YouLessMeter	m_meter;
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	volatile bool m_stoprequested;
	unsigned char m_PollCounter;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

