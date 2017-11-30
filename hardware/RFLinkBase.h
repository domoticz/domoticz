#pragma once

#include <vector>
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define RFLINK_READ_BUFFER_SIZE 65*1024

class CRFLinkBase: public CDomoticzHardwareBase
{
friend class CRFLinkSerial;
friend class CRFLinkTCP;
public:
	CRFLinkBase();
    ~CRFLinkBase();
	bool WriteToHardware(const char *pdata, const unsigned char length);
	virtual bool WriteInt(const std::string &sendString) = 0;
	bool m_bTXokay;
	bool m_bRFDebug;
	std::string m_Version;
private:
	void Init();
	void ParseData(const char *data, size_t len);
	bool ParseLine(const std::string &sLine);
	bool SendSwitchInt(const int ID, const int switchunit, const int BatteryLevel, const std::string &switchType, const std::string &switchcmd, const int level);
	unsigned char m_rfbuffer[RFLINK_READ_BUFFER_SIZE];
	int m_rfbufferpos;
	int m_retrycntr;
	time_t m_LastReceivedTime;
};

