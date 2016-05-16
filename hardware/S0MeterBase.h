#pragma once

#include "DomoticzHardware.h"

class S0MeterBase : public CDomoticzHardwareBase
{
	struct _tS0Meter{
		int m_type;
		double m_counter_start;
		double m_current_counter;
		double m_pulse_per_unit;
		unsigned long first_total_pulses_received;
		unsigned long total_pulses;
		int m_value_buffer_total;
		int m_value_buffer_write_pos;
		double m_last_values[5];
		double m_CurrentUsage;
		int m_PacketsSinceLastPulseChange;
		bool m_firstTime;
	};
public:
	S0MeterBase(void);
	~S0MeterBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
protected:
	void InitBase();
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void SendMeter(unsigned char ID, double musage, double mtotal);
	void ReloadLastTotals();
	unsigned char m_buffer[1028];

	static const int max_s0_meters=5;
	_tS0Meter m_meters[max_s0_meters];

	int m_bufferpos;
};

