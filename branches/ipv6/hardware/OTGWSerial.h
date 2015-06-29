#pragma once

#include "ASyncSerial.h"
#include "OTGWBase.h"

class OTGWSerial: public AsyncSerial, public OTGWBase
{
public:
	OTGWSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
    ~OTGWSerial();
private:
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();

	void StartPollerThread();
	void StopPollerThread();
	void Do_PollWork();
	bool WriteInt(const unsigned char *pData, const unsigned char Len);
	int m_retrycntr;
	boost::shared_ptr<boost::thread> m_pollerthread;
	volatile bool m_stoprequestedpoller;

    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
};

