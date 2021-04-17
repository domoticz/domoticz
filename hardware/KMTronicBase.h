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
	KMTronicBase();
	~KMTronicBase() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	virtual bool WriteInt(const unsigned char *data, size_t len, bool bWaitForReturn) = 0;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

      private:
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	unsigned char m_buffer[1028];
	int m_bufferpos;
	bool m_bDoInitialQuery;
	int m_TotRelais;
	std::array<bool, Max_KMTronic_Relais> m_bRelaisStatus;
};
