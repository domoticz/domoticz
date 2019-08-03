#pragma once

#include "ASyncSerial.h"
#include "RFLinkBase.h"

class CRFLinkSerial: public AsyncSerial, public CRFLinkBase
{
public:
	CRFLinkSerial(const int ID, const std::string& devname);
    ~CRFLinkSerial();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool WriteInt(const std::string &sendString) override;
	std::shared_ptr<std::thread> m_thread;
	std::string m_szSerialPort;
    void readCallback(const char *data, size_t len);
};

