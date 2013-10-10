#pragma once

#include "ASyncSerial.h"
#include "S0MeterBase.h"

class S0MeterSerial: public AsyncSerial, public S0MeterBase
{
public:
	S0MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int M1Type, const int M1PPH, const int M2Type, const int M2PPH);
    ~S0MeterSerial();

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

