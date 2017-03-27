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
	bool m_bDisableCRC;
	int m_ratelimit;

	unsigned long m_lastgasusage;
	time_t m_lastSharedSendGas;
	time_t m_lastUpdateTime;

	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	unsigned char m_CRfound;

	float m_voltagel1;
	float m_voltagel2;
	float m_voltagel3;

	void Init();
	bool MatchLine();
	void ParseData(const unsigned char *pData, const int Len, const bool disable_crc, int ratelimit);
	unsigned char m_buffer[1400];
	int m_bufferpos;
	unsigned char l_buffer[128];
	int l_bufferpos;
	unsigned char l_exclmarkfound;

	bool CheckCRC();
};
