#pragma once

#include "DomoticzHardware.h"
#include <iostream>

class CPVOutputInput : public CDomoticzHardwareBase
{
public:
	CPVOutputInput(const int ID, const std::string &SID, const std::string &Key);
	~CPVOutputInput(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	void SendTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void SendVoltage(const unsigned long Idx, const float Volt, const std::string &defaultname);
	void SendPercentage(const unsigned long Idx, const float Percentage, const std::string &defaultname);

	std::string m_SID;
	std::string m_KEY;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

