#pragma once

#include "DomoticzHardware.h"

class CBMP085 : public CDomoticzHardwareBase
{
public:
	CBMP085(const int ID);
	~CBMP085();
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void ReadSensorDetails();

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

	std::string m_ActI2CBus;

	bool i2c_test(const char *I2CBusName);
	int i2c_Open(const char *I2CBusName);
	int bmp_ReadInt(int fd, uint8_t *devValues, uint8_t startReg, uint8_t bytesToRead);
	int bmp_WriteCmd(int fd, uint8_t devAction);
	int bmp_Calibration(int fd);
	int WaitForConversion(int fd);
	int bmp_GetPressure(int fd, double *Pres);
	int bmp_GetTemperature(int fd, double *Temp);

	double   bmp_altitude(double p);
	double   bmp_qnh(double p, double StationAlt);
	double   ppl_DensityAlt(double PAlt, double Temp);

	// Calibration values - These are stored in the BMP085/180
	short int            bmp_ac1;
	short int            bmp_ac2;
	short int            bmp_ac3;
	unsigned short int   bmp_ac4;
	unsigned short int   bmp_ac5;
	unsigned short int   bmp_ac6;
	short int            bmp_b1;
	short int            bmp_b2;
	int                  bmp_b5;
	short int            bmp_mb;
	short int            bmp_mc;
	short int            bmp_md;
};
