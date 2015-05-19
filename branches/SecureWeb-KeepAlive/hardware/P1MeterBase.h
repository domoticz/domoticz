#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class P1MeterBase : public CDomoticzHardwareBase
{
	friend class P1MeterSerial;
	friend class P1MeterTCP;
public:
	P1MeterBase(void);
	~P1MeterBase(void);

	P1Power	m_p1power;
	P1Gas	m_p1gas;
private:
	static const int readBufferSize=1028;
	unsigned long m_lastgasusage;
	unsigned long m_lastelectrausage;
	time_t m_lastSharedSendElectra;
	time_t m_lastSharedSendGas;

	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	void Init();
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;

};

