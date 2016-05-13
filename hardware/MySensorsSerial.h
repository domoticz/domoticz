#pragma once

#include "ASyncSerial.h"
#include "MySensorsBase.h"

class MySensorsSerial: public AsyncSerial, public MySensorsBase
{
public:
	MySensorsSerial(const int ID, const std::string& devname, const int Mode1);
    ~MySensorsSerial();
private:
	bool StartHardware();
	bool StopHardware();

	unsigned int m_iBaudRate;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	void Do_Work();
	bool OpenSerialDevice();

    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
	void WriteInt(const std::string &sendStr);
};

