#pragma once

#include "ASyncSerial.h"
#include "KMTronicBase.h"

class KMTronicSerial: public AsyncSerial, public KMTronicBase
{
public:
	KMTronicSerial(const int ID, const std::string& devname);
    ~KMTronicSerial();
private:
	bool StartHardware();
	bool StopHardware();

	void GetRelayStates();
	int m_iQueryState;
	bool m_bHaveReceived;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	void Do_Work();
	bool OpenSerialDevice();

    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
	bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn);
};

