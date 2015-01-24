#pragma once

#include "ASyncSerial.h"
#include "MySensorsBase.h"

class MySensorsSerial: public AsyncSerial, public MySensorsBase
{
public:
	MySensorsSerial(const int ID, const std::string& devname);
    ~MySensorsSerial();

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

