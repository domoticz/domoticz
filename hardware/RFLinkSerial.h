#pragma once

#include <vector>
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
	bool WriteInt(const std::string &sendString);
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	std::string m_szSerialPort;
    void readCallback(const char *data, size_t len);
};

