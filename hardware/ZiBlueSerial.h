#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "ZiBlueBase.h"

class CZiBlueSerial: public AsyncSerial, public CZiBlueBase
{
public:
	CZiBlueSerial(const int ID, const std::string& devname);
    ~CZiBlueSerial();
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	bool WriteInt(const std::string &sendString);
	bool WriteInt(const uint8_t *pData, const size_t length);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	std::string m_szSerialPort;
    void readCallback(const char *data, size_t len);
};

