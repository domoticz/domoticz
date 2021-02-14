#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

class CDavisLoggerSerial : public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eDavisState
	{
		DSTATE_WAKEUP = 0,
		DSTATE_LOOP,
	};

      public:
	CDavisLoggerSerial(int ID, const std::string &devname, unsigned int baud_rate);
	~CDavisLoggerSerial() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void readCallback(const char *data, size_t len);
	bool HandleLoopData(const unsigned char *data, size_t len);
	bool OpenSerialDevice();
	void Do_Work();

      private:
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	_eDavisState m_state;
	int m_statecounter;
};
