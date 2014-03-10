#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CICYThermostat : public CDomoticzHardwareBase
{
public:
	CICYThermostat(const int ID, const std::string Username, const std::string Password);
	~CICYThermostat(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
private:
	void SendMeter(const unsigned char ID1,const unsigned char ID2, const double musage, const double mtotal, const std::string &defaultname);
	bool GetMeter(const unsigned char ID1,const unsigned char ID2, double &musage, double &mtotal);
	void SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);

	std::string m_UserName;
	std::string m_Password;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

