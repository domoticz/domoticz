#pragma once


#include "DomoticzHardware.h"

class C1Wire : public CDomoticzHardwareBase
{
public:
	C1Wire(const int ID);
	~C1Wire(void);

	static bool Have1WireSystem();
	static bool IsGPIOSystem();
	static bool IsOWFSSystem();
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	volatile bool m_stoprequested;
	time_t m_LastPollTime;
	boost::shared_ptr<boost::thread> m_thread;
	bool m_bIsGPIO;
	bool m_bIsOWFS;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetSensorDetails();
	void GetGPIOSensorDetails();
	void GetOWFSSensorDetails();
};
