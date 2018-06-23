#pragma once

#include "ASyncSerial.h"
#include "S0MeterBase.h"

class S0MeterSerial: public AsyncSerial, public S0MeterBase
{
public:
	S0MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate);
    ~S0MeterSerial();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
    void readCallback(const char *data, size_t len);
};

