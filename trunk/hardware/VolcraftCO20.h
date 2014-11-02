#pragma once

#ifndef WIN32
#ifdef WITH_LIBUSB
#include "DomoticzHardware.h"
#include <iostream>

class CVolcraftCO20 : public CDomoticzHardwareBase
{
public:
	CVolcraftCO20(const int ID);
	~CVolcraftCO20(void);

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	volatile bool m_stoprequested;
	time_t m_LastPollTime;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetSensorDetails();
};

#endif //WITH_LIBUSB
#endif //WIN32
