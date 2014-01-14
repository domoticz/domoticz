#pragma once

#include "DomoticzHardware.h"

class S0MeterBase : public CDomoticzHardwareBase
{
	struct _tS0Meter{
		int m_type;
		double m_pulse_per_unit;
		double m_last_values[4];
		double m_volume_total;
	};
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

	static const int max_s0_meters=5;
	_tS0Meter m_meters[max_s0_meters];

	int m_bufferpos;
};

