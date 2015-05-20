#pragma once

#include "DomoticzHardware.h"

#define Max_KMTronic_Relais 8

class KMTronicBase : public CDomoticzHardwareBase
{
	friend class KMTronicSerial;
	friend class KMTronicTCP;
	friend class KMTronic433;
public:
	KMTronicBase(void);
	~KMTronicBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	virtual bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn) = 0;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;

	bool m_bDoInitialQuery;
	int m_TotRelais;
	bool m_bRelaisStatus[Max_KMTronic_Relais];
};

