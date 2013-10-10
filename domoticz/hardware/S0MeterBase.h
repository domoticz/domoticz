#pragma once

#include "DomoticzHardware.h"

class S0MeterBase : public CDomoticzHardwareBase
{
	friend class S0MeterSerial;
	friend class S0MeterTCP;
public:
	S0MeterBase(void);
	~S0MeterBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
private:
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void SendMeter(unsigned char ID, double musage, double mtotal);
	void ReloadLastTotals();
	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;
	double m_s0_m1_last_values[4];
	double m_s0_m2_last_values[4];
	double m_s0_m1_volume_total;
	double m_s0_m2_volume_total;
	int m_s0_m1_type;
	int m_s0_m2_type;
	double m_pulse_per_unit_1;
	double m_pulse_per_unit_2;
};

