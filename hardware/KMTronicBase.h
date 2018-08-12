#pragma once

#include "DomoticzHardware.h"

#define Max_KMTronic_Relais 8

class KMTronicBase : public CDomoticzHardwareBase
{
	friend class KMTronicSerial;
	friend class KMTronicTCP;
	friend class KMTronic433;
	friend class KMTronicUDP;
public:
	KMTronicBase(void);
	~KMTronicBase(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	virtual bool WriteInt(const unsigned char *data, const size_t len, const bool bWaitForReturn) = 0;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
private:
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	unsigned char m_buffer[1028];
	int m_bufferpos;
	bool m_bDoInitialQuery;
	int m_TotRelais;
	bool m_bRelaisStatus[Max_KMTronic_Relais];
};

