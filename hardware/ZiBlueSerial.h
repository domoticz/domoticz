#pragma once

#include "ASyncSerial.h"
#include "ZiBlueBase.h"

class CZiBlueSerial: public AsyncSerial, public CZiBlueBase
{
public:
	CZiBlueSerial(const int ID, const std::string& devname);
    ~CZiBlueSerial();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool WriteInt(const std::string &sendString) override;
	bool WriteInt(const uint8_t *pData, const size_t length) override;
	std::shared_ptr<std::thread> m_thread;
	std::string m_szSerialPort;
    void readCallback(const char *data, size_t len);
};

