#pragma once

#include "DomoticzHardware.h"

class SolarEdgeBase : public CDomoticzHardwareBase
{
	enum _eSolarState
	{
		E_WAITING_SYNC=0,
		E_WAITING_LENGTH,
		E_WAITING_DATA,
	};
	friend class SolarEdgeTCP;
public:
	SolarEdgeBase(void);
	~SolarEdgeBase(void);
private:
	void ParseData(const unsigned char *pData, int Len);
	int ParsePacket(const unsigned char *pData, int len);
	//
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	void SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);

	float GetFloat(const unsigned char *pData);
	int ParsePacket0x0280(const unsigned char *pData, int dlen);
	int ParsePacket0x0281(const unsigned char *pData, int dlen);
	int ParsePacket0x0282(const unsigned char *pData, int dlen);
	int ParsePacket0x0283(const unsigned char *pData, int dlen);
	int ParsePacket0x0500(const unsigned char *pData, int dlen);
	//
	_eSolarState m_state;
	unsigned char m_RXBuffer[10*1024];
	int m_bufferpos;
	int m_wantedlength;
};

