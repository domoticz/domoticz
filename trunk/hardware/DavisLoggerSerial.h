#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

class CDavisLoggerSerial : public AsyncSerial, public CDomoticzHardwareBase
{
enum _eDavisState
{
	DSTATE_WAKEUP=0,
	DSTATE_LOOP,
};
public:
	CDavisLoggerSerial(const int ID, const std::string& devname, unsigned int baud_rate);
	~CDavisLoggerSerial(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void readCallback(const char *data, size_t len);
	bool HandleLoopData(const unsigned char *data, size_t len);
	void UpdateTempHumSensor(const unsigned char Idx, const float Temp, const int Hum);
	void UpdateTempSensor(const unsigned char Idx, const float Temp);
	void UpdateHumSensor(const unsigned char Idx, const int Hum);
	bool OpenSerialDevice();
	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;

	_eDavisState m_state;
	int m_statecounter;
};


