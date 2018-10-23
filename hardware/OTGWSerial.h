#pragma once

#include "ASyncSerial.h"
#include "OTGWBase.h"

class OTGWSerial: public AsyncSerial, public OTGWBase
{
public:
	OTGWSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
    ~OTGWSerial();
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();

	void Do_Work();
	bool WriteInt(const unsigned char *pData, const unsigned char Len) override;
	int m_retrycntr;
	std::shared_ptr<std::thread> m_thread;
    void readCallback(const char *data, size_t len);
};

