#pragma once

#include "DomoticzHardware.h"

class CBMP085 : public CDomoticzHardwareBase
{
public:
	CBMP085(const int ID);
	~CBMP085();

	unsigned int bmp085_GetTemperature();
	unsigned int bmp085_GetPressure();
	bool bmp085_Calibration();
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void ReadSensorDetails();
	int bmp085_i2c_Begin();
	unsigned int bmp085_ReadUT();
	unsigned int bmp085_ReadUP();

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_waitcntr;

	// Calibration values - These are stored in the BMP085
	short int ac1;
	short int ac2; 
	short int ac3; 
	unsigned short int ac4;
	unsigned short int ac5;
	unsigned short int ac6;
	short int b1; 
	short int b2;
	short int mb;
	short int mc;
	short int md;

	int b5; 
};
