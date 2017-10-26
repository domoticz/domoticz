#pragma once

#ifndef WIN32
#ifdef WITH_LIBUSB
#include "DomoticzHardware.h"
#include <iosfwd>

class CVolcraftCO20 : public CDomoticzHardwareBase
{
public:
	explicit CVolcraftCO20(const int ID);
	~CVolcraftCO20(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetSensorDetails();
};

#endif //WITH_LIBUSB
#endif //WIN32
