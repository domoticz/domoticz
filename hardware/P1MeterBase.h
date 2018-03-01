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

	P1Power	m_power;
	P1Gas	m_gas;
private:
	bool m_bDisableCRC;
	int m_ratelimit;

	unsigned char m_p1version;

	unsigned char m_buffer[1400];
	int m_bufferpos;
	unsigned char m_exclmarkfound;
	unsigned char m_linecount;
	unsigned char m_CRfound;

	unsigned char l_buffer[128];
	int l_bufferpos;
	unsigned char l_exclmarkfound;

	unsigned long m_lastgasusage;
	time_t m_lastSharedSendGas;
	time_t m_lastUpdateTime;

	float m_voltagel1;
	float m_voltagel2;
	float m_voltagel3;

	unsigned char m_gasmbuschannel;
	std::string m_gasprefix;
	std::string m_gastimestamp;
	double m_gasclockskew;
	time_t m_gasoktime;

	void Init();
	bool MatchLine();
	void ParseData(const unsigned char *pData, const int Len, const bool disable_crc, int ratelimit);

	bool CheckCRC();
};
