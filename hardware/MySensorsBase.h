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
	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;
};

