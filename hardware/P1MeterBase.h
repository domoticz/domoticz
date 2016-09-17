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
	unsigned long m_lastgasusage;
	unsigned long m_lastelectrausage;
	time_t m_lastSharedSendElectra;
	time_t m_lastSharedSendGas;

	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	unsigned char m_CRfound;
	void Init();
	bool MatchLine();
	void ParseData(const unsigned char *pData, int Len, unsigned char disable_crc);
	unsigned char m_buffer[1028];
	int m_bufferpos;
	unsigned char l_buffer[128];
	int l_bufferpos;
	unsigned char l_exclmarkfound;

	bool CheckCRC();
};

