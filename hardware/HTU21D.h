#pragma once

#include "DomoticzHardware.h"

class CHTU21D : public CDomoticzHardwareBase
{
public:
	explicit CHTU21D(const int ID);
	~CHTU21D();
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void ReadSensorDetails();

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	std::string m_ActI2CBus;

	bool i2c_test(const char *I2CBusName);
	int i2c_Open(const char *I2CBusName);
	int ReadInt(int fd, uint8_t *devValues, uint8_t bytesToRead);
	int WriteCmd(int fd, uint8_t devAction);
	int checkCRC8(uint16_t data);
	int GetHumidity(int fd, float *Pres);
	int GetTemperature(int fd, float *Temp);
};
