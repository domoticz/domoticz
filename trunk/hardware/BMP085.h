#pragma once

#include "DomoticzHardware.h"

class CBMP085 : public CDomoticzHardwareBase
{
public:
	CBMP085(const int ID);
	~CBMP085();

	int32_t bmp085_GetTemperature();
	unsigned int bmp085_GetPressure();
	bool bmp085_Calibration();
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void ReadSensorDetails();
	int bmp085_i2c_Begin();
	signed int bmp085_ReadUT();
	unsigned int bmp085_ReadUP();
	int32_t computeB5(int32_t UT);

	//Forecast
	int CalculateForecast(const float pressure);
	float m_LastPressure;
	int m_LastMinute;
	float m_pressureSamples[180];
	int m_minuteCount;
	bool m_firstRound;
	float m_pressureAvg[7];
	int m_LastForecast;
	unsigned char m_LastSendForecast;

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_waitcntr;

	// Calibration values - These are stored in the BMP085
	int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
	uint16_t ac4, ac5, ac6;
	int32_t b5;
};
