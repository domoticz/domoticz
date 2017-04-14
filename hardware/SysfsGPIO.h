/*
	This code implements basic I/O hardware via the Raspberry Pi's GPIO port, using sysfs.
*/
#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

class CSysfsGPIO : public CDomoticzHardwareBase
{
public:
	CSysfsGPIO(const int ID);
	~CSysfsGPIO();

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:
	bool StartHardware();
	bool StopHardware();
	void FindGpioExports();
	void Do_Work();
	void Init();
	void Poller_thread();
	void PollGpioInputs();
	void CreateDomoticzDevices();
	void UpdateDomoticzInputs();
	int GPIORead(int pin, const char* param);
	int GPIOReadFd(int fd);
	int GPIOWrite(int pin, int value);
	int GetReadResult(int bytecount, char* value_str);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	tRBUF m_Packet;
};
