#pragma once

#include "DomoticzHardware.h"

class MySensorsBase : public CDomoticzHardwareBase
{
	friend class MySensorsSerial;
public:
	MySensorsBase(void);
	~MySensorsBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
private:
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void UpdateSwitch(const unsigned char Idx, const int SubUnit, const bool bOn, const double Level, const std::string &defaultname);
	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;
};

