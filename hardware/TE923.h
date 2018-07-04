#pragma once

#ifndef WIN32

#ifdef WITH_LIBUSB

#include "DomoticzHardware.h"
#include <iosfwd>

class CTE923 : public CDomoticzHardwareBase
{
public:
	explicit CTE923(const int ID);
	~CTE923(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetSensorDetails();
private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
};

#endif //WITH_LIBUSB
#endif //WIN32
