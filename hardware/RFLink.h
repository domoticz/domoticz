#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define RFLINK_READ_BUFFER_SIZE 65*1024

class CRFLink: public AsyncSerial, public CDomoticzHardwareBase
{
public:
	CRFLink(const int ID, const std::string& devname);

    ~CRFLink();
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseLine(const std::string &sLine);
	bool SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level);

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	std::string m_szSerialPort;

	unsigned char m_rfbuffer[RFLINK_READ_BUFFER_SIZE];
	int m_rfbufferpos;
	int m_retrycntr;

	time_t m_LastReceivedTime;
	bool m_bTXokay;

//	boost::mutex m_sendMutex;
//	std::vector<std::string> m_sendqueue;

	/**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);

};

