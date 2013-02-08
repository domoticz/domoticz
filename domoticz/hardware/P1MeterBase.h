#pragma once

#include "DomoticzHardware.h"

typedef struct _tP1Power {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned long powerusage1;
	unsigned long powerusage2;
	unsigned long powerdeliv1;
	unsigned long powerdeliv2;
	unsigned long usagecurrent;
	unsigned long delivcurrent;
} P1Power;

typedef struct _tP1Gas {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	unsigned long gasusage;
} P1Gas;

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

