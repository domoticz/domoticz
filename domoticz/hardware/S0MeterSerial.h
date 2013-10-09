#pragma once

#include "ASyncSerial.h"
#include "S0MeterSerial.h"
#include "DomoticzHardware.h"

class S0MeterSerial: public AsyncSerial, public CDomoticzHardwareBase
{
public:
	S0MeterSerial(const int ID, const std::string& devname, const unsigned int baud_rate, const int M1Type, const int M1PPH, const int M2Type, const int M2PPH);
    ~S0MeterSerial();
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;

	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
    /**
     * Read callback, stores data in the buffer
     */
    void readCallback(const char *data, size_t len);
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void SendMeter(unsigned char ID, double musage, double mtotal);
	void ReloadLastTotals();
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

